/*-------------------------------------------------------------------------
* Copyright (C) 2020, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

import hudson.model.*

pipeline {
    parameters {
        string(name: 'SW_PLATFORM_BRANCH',
               defaultValue: 'origin/master',
               description: "SW platform branch to checkout")
        string(name: 'BRANCH',
               defaultValue: '$gitlabSourceBranch',
               description: "Branch name to checkout")
        string(name: 'NODE', defaultValue: 'AWS-Dispatcher', description: "Node label where the job should run")
    }
    agent { label "${params.NODE}" }
    post {
      failure {
        updateGitlabCommitStatus name: JOB_NAME, state: 'failed'
      }
      success {
        updateGitlabCommitStatus name: JOB_NAME, state: 'success'
        }
    }
    options {
        gitLabConnection('Gitlab')
        timeout(time: 3, unit: 'HOURS')
        buildDiscarder(logRotator(daysToKeepStr: '7', artifactDaysToKeepStr: '7'))
    }
    environment {
        // Params to pass to check if a child job has passed
        CHECK_CHILD_JOBS = " --commit-passed ${GIT_COMMIT}"
    }
    triggers {
        gitlab(triggerOnMergeRequest: true, branchFilterType: 'All')
    }
    stages {
        stage("Start") {
            steps {
                updateGitlabCommitStatus name: JOB_NAME, state: 'pending'
            }
        }
        stage('Checkout and Workspace Setup') {
            steps {
                sshagent (credentials: ['jennkis_aws_centos']) {
                    sh "git submodule deinit --all -f"
                    sh "rm -rf build install"
                    sh 'git clean -qxxxfd'
                    sh 'git submodule sync'
                    sh 'git reset --hard'
                }
                echo 'Git Checkout Done '
            }
        }
        // Trigger a SW platform build where we check that the host-sw commit does not
        // regress the integration in sw-platform
        stage('SW-Platform-Checkin-Top-Level') {
            parallel {
                stage('Runtime') {
                    steps {
                        build job: 'Software/sw-platform/component-builds/runtime',
                            parameters: [
                            string(name: 'NODE', value: "AWS"),
                            string(name: 'BRANCH', value: SW_PLATFORM_BRANCH),
                            string(name: 'COMPONENT_COMMITS',
                               value: "device-software/device-bootloaders:${BRANCH}"),
                            string(name: 'PARENT_JOB_NAME', value: JOB_NAME),
                            string(name: 'PARENT_BUILD_NUMBER', value: BUILD_NUMBER),
                            string(name: 'N_FAILED_TESTS', value: '2')
                        ]
                    }
                }
                stage('Glow-Integration-Top-Level') {
                    steps {
                        build job: 'Software/sw-platform/glow-rt-devfw-sysemu/rt-devfw-integration-top-level',
                            parameters: [
                            string(name: 'BRANCH', value: SW_PLATFORM_BRANCH),
                            string(name: 'COMPONENT_COMMITS',
                               value: "device-software/device-bootloaders:${BRANCH}"),
                            string(name: 'PARENT_JOB_NAME', value: JOB_NAME),
                            string(name: 'PARENT_BUILD_NUMBER', value: BUILD_NUMBER),
                            string(name: 'TIMEOUT', value: "5")
                        ]
                    }
                }
                stage('Zebu-Runs') {
                    steps {
                        build job: 'Software/sw-platform/Zebu/zebu-checkin-top-level',
                            parameters: [
                            string(name: 'BRANCH', value: SW_PLATFORM_BRANCH),
                            string(name: 'COMPONENT_COMMITS',
                               value: "device-software/device-bootloaders:${BRANCH}"),
                            string(name: 'PARENT_JOB_NAME', value: JOB_NAME),
                            string(name: 'PARENT_BUILD_NUMBER', value: BUILD_NUMBER),
                            string(name: 'TIMEOUT', value: "4")
                        ]
                    }
                }
            }
        }
    }
}
