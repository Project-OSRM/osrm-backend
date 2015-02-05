#-----------------------------------------------------------------------------
#
#  Configuration for YouCompleteMe Vim plugin
#
#  http://valloric.github.io/YouCompleteMe/
#
#-----------------------------------------------------------------------------

# some default flags
# for more information install clang-3.2-doc package and
# check UsersManual.html
flags = [
'-Werror',
'-Wall',
'-Wextra',
'-pedantic',
'-Wno-return-type',
'-Wno-unused-parameter',
'-Wno-unused-variable',

'-std=c++11',

# '-x' and 'c++' also required
# use 'c' for C projects
'-x',
'c++',

# libosmium include dirs
'-Iinclude',
'-Itest/include',
'-Itest/data-test/include',

# include third party libraries
'-I/usr/include/gdal',
]

# youcompleteme is calling this function to get flags
# You can also set database for flags. Check: JSONCompilationDatabase.html in
# clang-3.2-doc package
def FlagsForFile( filename ):
  return {
    'flags': flags,
    'do_cache': True
  }
