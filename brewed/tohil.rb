class Tohil < Formula
    include Language::Python::Virtualenv
    desc "Feathered serpent - delightful integration between Python and TCL"
    homepage "https://github.com/flightaware/tohil"
    url "https://github.com/flightaware/tohil/archive/refs/tags/v4.0.0.tar.gz"
    sha256 "dfa0805b86eb64c6755e01f3f2aaac0a8ef8e3181d3ebe8633de490ae73c252d"
    license "NOASSERTION"
    
    depends_on "python@3.9" 
    depends_on "tcl-tk" 
    depends_on "autoconf@2.69" => :build

  def install
    virtualenv_install_with_resources :using => "python@3.9"
    system "autoconf"
    system "./configure", "--prefix=/usr/local/opt", "--with-python-version=3.9", "--with-tcl=/usr/local/opt/tcl-tk/lib/"
    system "make"
    # system Formula["python@3.9"].opt_bin/"python3", *Language::Python.setup_install_args(prefix) 
    system "make", "install"
  end

  test do
    system "false"
  end
end
