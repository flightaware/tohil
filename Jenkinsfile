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

	def resultsdir = "results"
        stage('Build') {
		echo 'Building..'
		sh "printenv"
		sh "rm -fr ${resultsdir}"
		sh "mkdir -p ${resultsdir}"
		dir(srcdir) {
			sh "BRANCH=${env.BRANCH_NAME} pdebuild --use-pdebuild-internal --debbuildopts -b --buildresult ${WORKSPACE}/${resultsdir} -- --override-config"
		}
		archiveArtifacts artifacts: "${results}/*.deb", fingerprint: true
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
