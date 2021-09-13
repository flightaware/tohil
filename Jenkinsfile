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
        def dist = "buster"
        sh "rm -fr ${resultsdir}"
        sh "mkdir -p ${resultsdir}"
        dir(srcdir) {
            sh "DIST=${dist} BRANCH=${env.BRANCH_NAME} pdebuild --use-pdebuild-internal --debbuildopts -b --buildresult ${WORKSPACE}/${resultsdir}"
        }
        archiveArtifacts artifacts: "${results}/*.deb", fingerprint: true
        }

    stage("Test install on ${dist}") {
        sh "BRANCH=${env.BRANCH_NAME} /build/pi-builder/scripts/validate-packages.sh ${dist} ${resultsdir}/tohil_*.deb"
    }

    stage('Deploy') {
        steps {
            echo 'Deploying....'
        }
    }
}
