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
def GLOW_SHA = ""

pipeline {
    parameters {
        string(name: 'BRANCH',
               defaultValue: '$gitlabSourceBranch',
               description: "Branch name to checkout")
        string(name: 'NODE', defaultValue: 'AWS-Dispatcher', description: "Node label where the job should run")
        string(name: 'COMPONENT_COMMITS',
               defaultValue: "",
               description: "List of submodule-paths and their commits to checkout as part of the build. The formath is <SUBMODULE_PATH_1>:<COMMIT_1>,<SUBMODULE_PATH_2>:<COMMIT_2>")
        string(name: 'SW_PLATFORM_BRANCH',
               defaultValue: 'origin/master',
               description: "Branch of sw-platform to use")
    }
    agent { label "${params.NODE}" }
    post {
      aborted {
        updateGitlabCommitStatus name: JOB_NAME, state: 'canceled'
      }
      failure {
        updateGitlabCommitStatus name: JOB_NAME, state: 'failed'
      }
      success {
        updateGitlabCommitStatus name: JOB_NAME, state: 'success'
        }
    }
    options {
        gitLabConnection('Gitlab')
        timeout(time: 5, unit: 'HOURS')
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
                    sh "rm -rf build install"
                    sh 'git clean -qxxxfd'
                    sh 'git reset --hard'
                }
                echo 'Git Checkout Done '
            }
        }
        stage('ComponentBuild') {
            steps {
                build job: 'Software/sw-platform/component-builds/component-builder',
                    parameters: [
                    string(name: 'BRANCH', value: "${params.SW_PLATFORM_BRANCH}"),
                    string(name: 'COMPONENT_COMMITS', value: "device-software/test-compute-kernels:${GIT_COMMIT}"),
                    string(name: "PARENT_JOB_NAME", value: "${JOB_NAME}"),
                    string(name: "PARENT_BUILD_NUMBER", value: "${BUILD_NUMBER}"),
                    string(name: 'GIT_STEPS', value: "./CI/jenkins_job_runner.py ./device-software/test-compute-kernels/CI/checkin-regression.json GIT_STEPS"),
                    string(name: 'BUILD_STEPS', value: "./CI/jenkins_job_runner.py ./device-software/test-compute-kernels/CI/checkin-regression.json BUILD_STEPS"),
                    string(name: 'TEST_STEPS', value: "./CI/jenkins_job_runner.py ./device-software/test-compute-kernels/CI/checkin-regression.json TEST_STEPS")
                ]
            }
        }
    }
}
