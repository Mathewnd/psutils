project(
	'psutils',
	'c',
	version: '0',
	default_options: [
		'c_std=gnu17',
		'warning_level=2',
	]
)

psutils_include = include_directories('include')

install_headers('include/psutils/psutils.h', subdir: 'psutils')

subdir('lib')
subdir('programs')
