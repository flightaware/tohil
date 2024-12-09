node(label: 'raspberrypi') {
    properties([
            disableConcurrentBuilds(),
            durabilityHint(hint: 'PERFORMANCE_OPTIMIZED')
        ])

    def srcdir = "${WORKSPACE}/src"
    def resultsdir = "results"
    def dist = "bookworm"
    def pkgdir = "package-${dist}"

    stage("Checkout") {
        sh "rm -fr ${srcdir}"
        sh "mkdir ${srcdir}"
        dir(srcdir) {
            checkout scm
        }
    }

    stage("Prepare source for ${dist}") {
        sh "rm -fr ${pkgdir}"
        sh "${srcdir}/prepare-build.sh ${dist} ${pkgdir}"
    }

    stage("Build") {
        sh "rm -fr ${resultsdir}"
        sh "mkdir -p ${resultsdir}"
        dir(srcdir) {
            sh "DIST=${dist} BRANCH=${env.BRANCH_NAME} pdebuild --logfile debug.log  --use-pdebuild-internal --debbuildopts -b --buildresult ${WORKSPACE}/${resultsdir}"
        }
        archiveArtifacts artifacts: "${resultsdir}/*.deb", fingerprint: true
    }

    stage("Test install on ${dist}") {
        sh "BRANCH=${env.BRANCH_NAME} /build/pi-builder/scripts/validate-packages.sh ${dist} ${resultsdir}/python3-tohil_*.deb"
    }

    stage("Deployment to internal repository") {
        sh "/build/pi-builder/scripts/deploy.sh -distribution ${dist} -branch ${env.BRANCH_NAME} ${resultsdir}/*.deb"
    }
}
