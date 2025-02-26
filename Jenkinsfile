node(label: 'raspberrypi') {
    properties([
            disableConcurrentBuilds(),
            durabilityHint(hint: 'PERFORMANCE_OPTIMIZED')
        ])

    def dists = ["bookworm","bullseye"]
    def srcdir = "${WORKSPACE}/src"

    for (int i = 0; i < dists.size(); ++i) {
        def dist = dists[i]
        def pkgdir = "package-${dist}"
        def resultsdir = "results-${dist}"

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

        stage("Build for ${dist}") {
            sh "rm -fr ${resultsdir}"
            sh "mkdir -p ${resultsdir}"
            dir(pkgdir) {
                sh "DIST=${dist} BRANCH=${env.BRANCH_NAME} pdebuild --use-pdebuild-internal --debbuildopts -b --buildresult ${WORKSPACE}/${resultsdir} -- --override-config"
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
}
