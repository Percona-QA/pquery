pipeline {
    agent { label 'min-bionic-x64' }
    parameters {
        choice(
            choices: 'OFF\nON',
            description: '',
            name: 'WITH_MYSQL')
        choice(
            choices: 'OFF\nON',
            description: '',
            name: 'WITH_PGSQL')
    }
        stages {
            stage('git checkout') {
                steps {
                    step([$class: 'WsCleanup'])
                    git url: 'https://github.com/Percona-QA/pquery.git', branch: '${BRANCH}'
                }
            }
            stage('apt-get update') {
                steps {
                    sh 'sudo apt-get update'
                }
            }
            stage('install build tools') {
                steps {
                    sh 'sudo apt-get -y install cmake make g++ gcc'
                }
            }
            stage('install common deps') {
                steps {
                    sh 'sudo apt-get -y install libssl-dev zlib1g-dev'
                }
            }
            stage('install MySQL deps') {
                when { expression { params.WITH_MYSQL == 'ON' } }
                steps {
                    sh 'sudo apt-get -y install libmysqlclient-dev'
                }
            }
                        stage('install PgSQL deps') {
                when { expression { params.WITH_PGSQL == 'ON' } }
                steps {
                    sh 'sudo apt-get -y install libpq-dev'
                }
            }
            stage('cmake config') {
                steps {
                    sh 'cmake . -DWITH_MYSQL=${WITH_MYSQL} -DWITH_PGSQL=${WITH_PGSQL} -DDEVELOPER_MODE=${DEVELOPER_MODE}'
                }
            }
            stage('build binary') {
                steps {
                    sh 'make'
                }
            }
        }
    }
