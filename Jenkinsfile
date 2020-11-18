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
                    sh 'apt-get -yyy --no-install-recommends install \
                          build-essential \
                          cmake \
                          gcc \
                          g++ \
                          gdb \
                          git-core \
                          libcurl4-openssl-dev'
                    sh 'cd ingest-tools && cmake CMakeLists.txt && make'
                }
            }
        }
    }
}

