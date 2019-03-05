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
    }
    agent { label "AWS-Docker-Image-Builder" }
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
                        url: 'git@gitlab:software/device-firmware.git'
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
                // Build the base image first we inherit from and push
                sh './device-firmware.py --logging=DEBUG docker build-image et-sw-base'
                sh './device-firmware.py --logging=DEBUG docker push-image et-sw-base'
                // Build the sw-platform image
                sh './device-firmware.py --logging=DEBUG docker build-image et-sw-device-fw'
                sh './device-firmware.py --logging=DEBUG docker push-image et-sw-device-fw'
                withDockerRegistry([ credentialsId: "jenkins-nexus3", url: "https://sc-docker-registry.esperanto.ai/v2/repository/et-sw" ]) {
                    echo "Build Docker For Colo"
                    // Build the base image first we inherit from and push
                    sh './device-firmware.py --logging=DEBUG docker build-image --docker-registry Colo et-sw-base'
                    sh './device-firmware.py --logging=DEBUG docker push-image --docker-registry Colo et-sw-base'
                    // Build the sw-platform image
                    sh './device-firmware.py --logging=DEBUG docker build-image --docker-registry Colo et-sw-device-fw'
                    sh './device-firmware.py --logging=DEBUG docker push-image --docker-registry Colo et-sw-device-fw'
                }
                echo "Building Docker Image done"
            }
        }
        stage("Docker remove old images") {
            steps {
            // Remove any old images that have not been used for a long time
            // Do it here instead of the beginning to avoid throwing away
            // any usefull cached layers
            echo "Docker remove old images"
            sh 'docker image prune -a --force --filter "until=240h"'
                sh 'docker system prune -f'
            }
        }
    }
}
