#
# helper procs for tohil
#

namespace eval ::tohil {
	proc run {pythonCode} {
		return [tohil::call tohil.run $pythonCode]
	}

	# this is a tcl channel implementation that
	# if pushed onto a tcl channel will write what
	# that channel gets to python's stdout
	namespace eval pychan {
		namespace ensemble create -map {
			initialize init
			finalize close
			write write
		}

		proc init {id mode} {
			return {initialize finalize write}
		}

		proc finalize {id} {
		}

		proc write {id data} {
			#tohil::call __builtins__.print $data
			tohil::call sys.stdout.write $data
			#return [string bytelength $data]
			return
		}
	}

	#
	# redirect_stdout_to_python - redirect tcl's stdout
	#   to go to python's stdout by pushing our little
	#   pychan channel handler onto tcl's stdout
	#
	proc redirect_stdout_to_python {} {
		::tohil::import sys
		chan push stdout ::tohil::pychan
		return
	}
}

proc whee {python} {
	return [::tohil::exec $python]
}

proc tohil_rivet {} {
	tohil::exec "import tohil; tohil.rivet()"
}

