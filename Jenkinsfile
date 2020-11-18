pipeline {
    agent {
        kubernetes {
            containerTemplate {
                name 'ubuntu'
                image 'ubuntu:20.04'
                command 'sleep'
                args 'infinity'
            }
        }
    }
    stages {
        stage('test') {
            steps {
                container('ubuntu') {
                    sh 'apt-get update'
                    sh 'apt-get apt-get -y --no-install-recommends install \
                          aptitude \
                          bash-completion \
                          build-essential \
                          cmake \
                          coreutils \
                          gcc \
                          g++ \
                          gdb \
                          git-core \
                          ncdu \
                          unzip \
                          vim \
                          libcurl4-openssl-dev'
                    sh 'cd fmp4-ingest/ingest-tools && cmake CMakeLists.txt && make'
                }
            }
        }
    }
}

