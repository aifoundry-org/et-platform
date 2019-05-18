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

def trenary_op(String a, String b){
    if (a.length() > 0) {
        return a
    }
    return b
}

pipeline {
    parameters {
        string(name: 'BRANCH',
               defaultValue: '$gitlabSourceBranch',
               description: "Branch name to checkout")
        string(name: 'PARENT_JOB_NAME', defaultValue: '', description: 'Name of the parent build.')
        string(name: 'PARENT_BUILD_NUMBER', defaultValue: '', description: 'ID of the parent build.')
        string(name: 'NODE', defaultValue: 'AWS', description: "Node label where the job should run")
        string(name: 'GIT_STEPS',
               defaultValue: './CI/jenkins_job_runner.py ./CI/$JOB_NAME.json GIT_STEPS',
               description: "Additional GIT steps to perform")
        string(name: 'BUILD_STEPS',
               defaultValue: './CI/jenkins_job_runner.py ./CI/$JOB_NAME.json BUILD_STEPS',
               description: "Build steps to perform")
        string(name: 'TEST_STEPS',
               defaultValue: './CI/jenkins_job_runner.py ./CI/$JOB_NAME.json TEST_STEPS',
               description: "Test steps to perform")
    }
    agent { label "${params.NODE}" }
    environment {
        NAME = trenary_op PARENT_JOB_NAME, JOB_NAME
        ID = trenary_op PARENT_BUILD_NUMBER, BUILD_NUMBER
    }
    post {
      failure {
        updateGitlabCommitStatus name: JOB_NAME, state: 'failed'
      }
      success {
        updateGitlabCommitStatus name: JOB_NAME, state: 'success'
        }
        always {
            sh "./device-firmware.py et-sw-device-fw artifacts.py --push ./build/artifacts/ et-sw-ci/${NAME}/${ID}/${GIT_COMMIT}"
        }
    }
    options {
        gitLabConnection('Gitlab')
        timeout(time: 1, unit: 'HOURS')
    }
    triggers {
        gitlab(triggerOnMergeRequest: true, branchFilterType: 'All')
    }
    stages {
        stage('Checkout') {
            steps {
                checkout([
                    $class: 'GitSCM',
                    branches: [[name: BRANCH]],
                    doGenerateSubmoduleConfigurations: false,
                    extensions: [[$class: 'CleanCheckout']],
                    submoduleCfg: [],
                    userRemoteConfigs: [[
                        credentialsId: 'jennkis_aws_centos',
                        url: 'git@gitlab.esperanto.ai:software/device-firmware.git'
                    ]]
                ])

                sshagent (credentials: ['jennkis_aws_centos']) {
                    sh 'git clean -xfd'
                    sh 'git submodule foreach --recursive git clean -xfd'
                    sh 'git reset --hard'
                    sh 'git submodule foreach --recursive git reset --hard'
                    sh 'git submodule sync'
                    sh 'git submodule update --init infra_tools'
                }
                echo 'Git Checkout Done '
            }
        }

        stage('Docker-Build') {
            steps {
                echo "Building Docker Image"
                sh "rm -rf build/ "
                build job: 'Software/device-fw/docker-image-builder',
                    parameters: [
                        string(name: 'BRANCH', value: BRANCH)
                    ]
                echo "Building Docker Image dONE"
            }
        }

        stage('Git-Actions') {
            steps {
                echo "Git-Actions"
                sh "rm -rf build/ "
                sh "rm -rf install"
                sh "${params.GIT_STEPS}"
                echo "Git-Actions Done"
            }
        }
        stage('Build') {
            steps {
                echo "Building"
                sh "${params.BUILD_STEPS}"
                echo "Building Done"
            }
        }
        stage('Test') {
            steps {
                echo "Test"
                sh "${params.TEST_STEPS}"
                echo "Testing Done"
            }
        }
    }
}
