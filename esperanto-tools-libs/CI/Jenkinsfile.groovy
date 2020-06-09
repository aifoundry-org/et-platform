/*-------------------------------------------------------------------------
* Copyright (C) 2019,2020 Esperanto Technologies Inc.
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

def launch_job(String SW_PLATFORM_BRANCH, String BRANCH, String VariantName, String Config) {
    build job: 'Software/sw-platform/component-builds/component-builder',
        parameters: [
        string(name: 'BRANCH', value: "${SW_PLATFORM_BRANCH}"),
        string(name: 'COMPONENT_COMMITS',
               value: "host-software/esperanto-tools-libs:${BRANCH}"),
        string(name: "PARENT_JOB_NAME",
               value: "${JOB_NAME}-${VariantName}"),
        string(name: "PARENT_BUILD_NUMBER",
               value: "${BUILD_NUMBER}"),
        string(name: 'GIT_STEPS',
               value: "./CI/jenkins_job_runner.py ./host-software/esperanto-tools-libs/CI/${Config} GIT_STEPS"),
        string(name: 'BUILD_STEPS',
               value: "./CI/jenkins_job_runner.py ./host-software/esperanto-tools-libs/CI/${Config} BUILD_STEPS"),
        string(name: 'TEST_STEPS',
               value: "./CI/jenkins_job_runner.py ./host-software/esperanto-tools-libs/CI/${Config} TEST_STEPS")
    ]
}

pipeline {
    parameters {
        string(name: 'BRANCH',
               defaultValue: '$gitlabSourceBranch',
               description: "Branch name to checkout")
        string(name: 'SW_PLATFORM_BRANCH',
               defaultValue: 'master',
               description: "Branch of sw-platform to use")
        string(name: 'PARENT_JOB_NAME', defaultValue: '', description: 'Name of the parent build.')
        string(name: 'PARENT_BUILD_NUMBER', defaultValue: '', description: 'ID of the parent build.')
        string(name: 'NODE', defaultValue: 'AWS-Dispatcher', description: "Node label where the job should run")
        // string(name: 'GIT_STEPS',
        //        defaultValue: './CI/jenkins_job_runner.py ./CI/$JOB_NAME.json GIT_STEPS',
        //        description: "Additional GIT steps to perform")
        // string(name: 'BUILD_STEPS',
        //        defaultValue: './CI/jenkins_job_runner.py ./CI/$JOB_NAME.json BUILD_STEPS',
        //        description: "Build steps to perform")
        // string(name: 'TEST_STEPS',
        //        defaultValue: './CI/jenkins_job_runner.py ./CI/$JOB_NAME.json TEST_STEPS',
        //        description: "Test steps to perform")
    }
    agent { label "${params.NODE}" }
    options {
        gitLabConnection('Gitlab')
        timeout(time: 2, unit: 'HOURS')
    }
    triggers {
        gitlab(triggerOnMergeRequest: true, branchFilterType: 'All')
    }
    post {
        failure {
            updateGitlabCommitStatus name: "${JOB_NAME}", state: 'failed'
        }
        success {
            updateGitlabCommitStatus name: "${JOB_NAME}", state: 'success'
        }
    }
    stages {
        stage("Setup") {
            steps {
                updateGitlabCommitStatus name: "${JOB_NAME}", state: 'pending'
            }
        }
        stage("Build Variants") {
            parallel {
                stage("Checkin") {
                    steps {
                        script {
                            launch_job("${params.SW_PLATFORM_BRANCH}", "${BRANCH}", "Checkin", "Runtime-checkin.json")
                        }
                    }
                }
                /*  SW-3188
                stage("Address Sanitizer") {
                    steps {
                        script {
                            launch_job("${params.SW_PLATFORM_BRANCH}", "${BRANCH}", "AddressSanitizer", "Runtime-address-sanitizer.json")
                        }
                    }
                }

                stage("Thread Sanitizer") {
                    steps {
                        script {
                            launch_job("${params.SW_PLATFORM_BRANCH}", "${BRANCH}", "ThreadSanitizer", "Runtime-thread-sanitizer.json")
                        }
                    }
                }

                stage("Memory Sanitizer") {
                    steps {
                        script {
                            launch_job("${params.SW_PLATFORM_BRANCH}", "${BRANCH}", "MemorySanitizer", "Runtime-memory-sanitizer.json")
                        }
                    }
                }

                 */

                stage("Undefined Sanitizer") {
                    steps {
                        script {
                            launch_job("${params.SW_PLATFORM_BRANCH}", "${BRANCH}", "UndefinedSanitizer", "Runtime-undefined-sanitizer.json")
                        }
                    }
                }

                stage("Coverage") {
                    steps {
                        script {
                            launch_job("${params.SW_PLATFORM_BRANCH}", "${BRANCH}", "Coverage", "Runtime-Coverage.json")
                        }
                    }
                }

            }
        }

        stage('Integration Smoke Test') {
            steps {
                build job: 'Software/sw-platform/checkin-regressions/integration-smoketest',
                    parameters: [
                    string(name: 'NODE', value: "AWS"),
                    string(name: 'BRANCH', value: "${params.SW_PLATFORM_BRANCH}"),
                    string(name: 'COMPONENT_COMMITS',
                           value: "host-software/esperanto-tools-libs:${BRANCH}"),
                    string(name: "PARENT_JOB_NAME",
                           value: "${JOB_NAME}"),
                    string(name: "PARENT_BUILD_NUMBER",
                           value: "${BUILD_NUMBER}")
                ]
            }
        }
    }
}
