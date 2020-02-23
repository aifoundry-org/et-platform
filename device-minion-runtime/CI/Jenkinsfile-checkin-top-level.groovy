/*-------------------------------------------------------------------------
* Copyright (C) 2019, Esperanto Technologies Inc.
* The copyright to the computer program(s) herein is the
* property of Esperanto Technologies, Inc. All Rights Reserved.
* The program(s) may be used and/or copied only with
* the written permission of Esperanto Technologies and
* in accordance with the terms and conditions stipulated in the
* agreement/contract under which the program(s) have been supplied.
*-------------------------------------------------------------------------
*/

import hudson.model.*

// Run Zebu inside sw-platform, false by default
def run_zebu = false

pipeline {
    parameters {
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
        buildDiscarder(logRotator(daysToKeepStr: '15', artifactDaysToKeepStr: '15'))
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
        stage('Device-FW-checkin') {
            /* FIXME SW-20208 we should introduce a versioned utility docker image that will enable the jenkins_jobs.py scrupt to work
            when {
                // Trigger the build if it has not passed before for this commit
                expression {
                    return sh(returnStatus: true, script: """ \
                            ./device-firmware.py et-sw-device-fw "./CI/jenkins_jobs.py ${CHECK_CHILD_JOBS} checkin-regression "\
                         """) != 0
                }
            }
             */
            steps {
                build job: 'Software/sw-platform/component-builds/device-software/device-minion-runtime/checkin-regression',
                    parameters: [
                    string(name: 'NODE', value: "AWS"),
                    string(name: 'BRANCH', value: BRANCH),
                    string(name: 'PARENT_JOB_NAME', value: JOB_NAME),
                    string(name: 'PARENT_BUILD_NUMBER', value: BUILD_NUMBER)
                ]
            }
        }
        // Trigger a SW platform build where we check that the host-sw commit does not
        // regress the integration in sw-platform
        stage('SW-Platform-Checkin-Top-Level') {
            steps {
                build job: 'Software/sw-platform/checkin-top-level',
                    parameters: [
                    string(name: 'BRANCH', value: "origin/master"),
                    string(name: 'COMPONENT_COMMITS',
                           value: "device-software/device-minion-runtime:${GIT_COMMIT}"),
                    string(name: 'PARENT_JOB_NAME', value: JOB_NAME),
                    string(name: 'PARENT_BUILD_NUMBER', value: BUILD_NUMBER),
                ]
            }
        }
    }
}
