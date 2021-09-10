node(label: 'raspberrypi') {
	properties([
            disableConcurrentBuilds(),
            durabilityHint(hint: 'PERFORMANCE_OPTIMIZED')
        ])

	def srcdir = "${WORKSPACE}/src"
	stage('Checkout') {
            sh "rm -fr ${srcdir}"
            sh "mkdir ${srcdir}"
            dir(srcdir) {
                checkout scm
            }
        }

        stage('Build') {
            steps {
                echo 'Building..'
            }
        }
        stage('Test') {
            steps {
                echo 'Testing..'
            }
        }
        stage('Deploy') {
            steps {
                echo 'Deploying....'
            }
        }
}
