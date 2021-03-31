#
# helper procs for tohil
#

namespace eval ::tohil {
	proc run {pythonCode} {
		return [tohil::call tohil.run $pythonCode]
	}
}

proc whee {python} {
	return [::tohil::exec $python]
}

proc tohil_rivet {} {
	tohil::exec "import tohil; tohil.rivet()"
}
