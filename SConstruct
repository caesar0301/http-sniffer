#!/usr/bin/evn python
env = Environment(CCFLAGS='-w -g')
lib_path = ['/usr/local/lib', '/usr/lib']
libs = Glob('./src/*.a') + ['pthread', 'pcap']
cpp_path=['./src']

# Flag debug to decide if NFM libs are used to compile program
debug = ARGUMENTS.get('--enable-nfm', 0)
if int(debug):
	# NFM library names
	nfm_libs = ['nfm', 'nfm_framework', 'nfm_error', 'nfm_packet', 'nfm_rules', 'nfm_platform', 'nfe', 'nfp']
	libs += nfm_libs
	# NFM library path
	lib_path.append('/opt/netronome/lib')
	# NFM header file path
	cpp_path.append('/opt/netronome/nfm/include')

# Compile the programs
env.Program(target = './runhs', 
			source = Glob('./src/*.c'),
			LIBPATH = lib_path,
			LIBS = libs,
			CPPPATH = cpp_path)