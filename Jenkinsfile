
// Uses Declarative syntax to run commands inside a container. 
pipeline {
    agent {
        kubernetes {
            // Define any agents required for this pipeline
            yamlFile 'kubernetesAgents.yaml'
        }
    }
    environment {
        // Name
        NAME = 'fmp4-ingest'
        // Generally useful environment variables
        KUBECONFIG = credentials('kubeconfig')
        REGISTRY_TOKEN = credentials('gitlab-registry-operations')
        GIT_COMMIT = sh(returnStdout: true, script: 'git rev-parse HEAD').trim()
        GIT_COMMIT_SHORT = sh(returnStdout: true, script: 'git rev-parse --short HEAD').trim()
        CHART_CREDS = credentials('chartmuseum')
        CHART_REPO = "http://$CHART_CREDS_USR:$CHART_CREDS_PSW@$CHART_REPO_URL"
        // Docker and Helm deployment stuff
        DOCKER_REPO_BASE = "$REGISTRY_URL/operations/base-images/$NAME"
        DOCKER_CACHE = "$DOCKER_REPO_BASE/cache"
        DOCKER_REPO = "$DOCKER_REPO_BASE/$BRANCH_NAME"
        // Publishing to GitHub and Docker Hub
        PUBLISH = 'false'
        //GITHUB_CREDENTIALS = 'jenkins-ssh-key-github'
        //GITHUB_REPO = "github.com:unifiedstreaming/fmp4-ingest.git"
        DOCKER_HUB_REGISTRY_TOKEN = credentials('docker-hub')
        DOCKER_HUB_REGISTRY_URL = 'docker.io'
        DOCKER_HUB_REPO = "$DOCKER_HUB_REGISTRY_URL/unifiedstreaming/fmp4-ingest"
    }
    options {
        quietPeriod(5)
    }
    parameters {
        string(name: 'ALPINEVERSION', defaultValue: '3.15', description: 'Alpine version to use (default 3.15)')
    }
    stages {
        stage('Set build name') {
            steps {
                script {
                    currentBuild.displayName = "#${currentBuild.id} - git ${env.GIT_COMMIT}"
                }
            }
        }
        stage('Build Docker image') {
            steps {
                container('kaniko') {
                    sh 'echo "{\\"auths\\":{\\"$REGISTRY_URL\\":{\\"username\\":\\"$REGISTRY_TOKEN_USR\\",\\"password\\":\\"$REGISTRY_TOKEN_PSW\\"}}}" > /kaniko/.docker/config.json'
                    sh """
                        /kaniko/executor \
                            -f `pwd`/docker/fmp4-ingest/Dockerfile \
                            -c `pwd`/docker/fmp4-ingest \
                            --cache=true \
                            --cache-repo=$DOCKER_CACHE \
                            --build-arg ALPINEVERSION=${params.ALPINEVERSION} \
                            --destination $DOCKER_REPO:$GIT_COMMIT \
                            --destination $DOCKER_REPO:latest \
                    """
                }
            }
        }
        stage('Publish Helm chart') {
            steps {
                container('helm') {
                    sh '''
                        sed -i \
                            -e "s|tag: latest|tag: $GIT_COMMIT|g" \
                            -e "s|repository: .*|repository: $DOCKER_REPO|g" \
                            chart/values.yaml
                        sed -i \
                            -e "s|version: 0.0.0-trunk|version: 0.0.0-trunk-$GIT_COMMIT_SHORT|g" \
                            -e "s|appVersion: 0.0.0-trunk|appVersion:  0.0.0-trunk-$GIT_COMMIT_SHORT|g" \
                            chart/Chart.yaml
                        helm --kubeconfig $KUBECONFIG \
                            push \
                            ./chart \
                            $CHART_REPO
                    '''
                }
            }
        }
        stage('Deploy and Test image with Live Demo') {
            steps {
                script {
                if (env.BRANCH_NAME == 'trunk') {
                    build job: 'demo/live/trunk', parameters: [string(name: 'FMP4INGEST_VERSION', value: "0.0.0-trunk-$GIT_COMMIT_SHORT")], wait: true
                }
              }
            }
        }
        //stage('Publish to GitHub') {
        //    when {
        //        allOf {
        //            environment name: 'PUBLISH', value: 'true'
        //            environment name: 'BRANCH_NAME', value: 'stable'
        //        }
        //    }
        //    steps {
        //        sh 'mkdir -p github'
        //        dir('github') {
        //            git branch: env.RELEASE_OR_BRANCH, credentialsId: env.GITHUB_CREDENTIALS, url: env.GITHUB_REPO
        //            sh 'rm -rf *'
        //            sh 'cp -R ../docker .'
        //            sh 'cp ../README.public.md ./README.md || cp ../README.md .'
        //            sh 'cp ../my_use_case.yaml .'
        //            sh 'cp ../unifiedstreaming-logo-black.png .'
        //            sh 'git add .'
        //            sh 'git config user.name "Unified Streaming"'
        //            sh 'git config user.email "ops@unified-streaming.com"'
        //            sh 'git commit -m "Publish $VERSION" || return 0'
        //            sh "git remote set-url origin git@${env.GITHUB_REPO}"
        //            sshagent (credentials: [env.GITHUB_CREDENTIALS]) {
        //                script {
        //                    if (env.RELEASE_OR_BRANCH == 'stable') {
        //                        sh 'git tag $VERSION || git tag --force $VERSION && git push --delete origin refs/tags/$VERSION'
        //                    } else {
        //                        sh 'git tag $VERSION-beta || git tag --force $VERSION-beta && git push --delete origin refs/tags/$VERSION-beta'
        //                    }
        //                }
        //                sh "git push origin ${env.RELEASE_OR_BRANCH}"
        //                sh 'git push origin --tags'
        //            }
        //        }
        //    }
        //}
        //stage('Publish Docker image') {
        //    when {
        //        anyOf {
        //            environment name: 'RELEASE_OR_BRANCH', value: 'stable'
        //        }
        //    }
        //    steps {
        //        container('crane') {
        //            sh 'crane auth login -u $REGISTRY_TOKEN_USR -p $REGISTRY_TOKEN_PSW $REGISTRY_URL'
        //            sh 'crane auth login -u $DOCKER_HUB_REGISTRY_TOKEN_USR -p $DOCKER_HUB_REGISTRY_TOKEN_PSW $DOCKER_HUB_REGISTRY_URL'
        //            script {
        //                if (env.RELEASE_OR_BRANCH == 'stable') {
        //                    sh 'crane copy $DOCKER_REPO:$VERSION $DOCKER_HUB_REPO:$VERSION'
        //                    sh 'crane copy $DOCKER_REPO:$VERSION $DOCKER_HUB_REPO:latest'
        //                } else {
        //                    sh 'crane copy $DOCKER_REPO:$VERSION $DOCKER_HUB_REPO:$VERSION-beta'
        //                }
        //            }
        //            
        //        }
        //    }
        //}
        //stage('Update Docker Hub readme') {
        //    when {
        //        environment name: 'RELEASE_OR_BRANCH', value: 'stable'
        //    }
        //    steps {
        //        container('docker-pushrm') {
        //            sh '''
        //                export DOCKER_USER=$DOCKER_HUB_REGISTRY_TOKEN_USR
        //                export DOCKER_PASS=$DOCKER_HUB_REGISTRY_TOKEN_PSW
        //                /docker-pushrm --file ./github/README.md --debug $DOCKER_HUB_REPO
        //            '''
        //        }
        //    }
        //}
    }
    post {
        fixed {
            slackSend (color: '#00FF00', message: "FIXED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
        }
        failure {
            slackSend (color: '#FF0000', message: "FAILED: Job '${env.JOB_NAME} [${env.BUILD_NUMBER}]' (${env.BUILD_URL})")
        }
        //success {
        //    script {
        //        if (env.RELEASE_OR_BRANCH == 'trunk') {
        //            build job: 'demo/live/trunk', parameters: [string(name: 'VERSION', value: "$VERSION"), string(name: 'SVN_COMMIT', value: "$ACTUAL_SVN_COMMIT")], wait: false
        //        }
        //    }
        //}
    }
}
