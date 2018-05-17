# What is pquery?
pquery is an open-source (GPLv2 licensed) multi-threaded test program created to stress test the MySQL server (in any flavor), either randomly or sequentially, for QA purposes. Given it's modern C++ core, it is able to maximise the physical server's queries per second (qps) rate. pquery is an acronym for 'parallel query'. Prebuilt pquery binaries (with statically linked client libraries) for Percona Server, MySQL Server, MariaDB, and WebScaleSQL are available as part of the pquery framework.

+ *pquery v1.x* was designed for single-node MySQL setup, and accepts command line options only. Ref ```pquery -help``` (v1.0 only)
+ *pquery v2.x* was designed for multi-node MySQL setups, and accepts command line options, as well as options from a configuration file in INI format. Ref ```pquery --config-help``` (v2.0 only)
+ *pquery v3.x* was designed for multi-node MySQL and PostgreSQL setups, it accepts options from a configuration file in INI format. Ref ```pquery -h```

Please note that v2.0 accepts the same CLI options as v1.0 does, for backwards compatibility. And, alike to v1.0, it can handle a single node setup in that mode. The recommended way to pass all options and params to pquery v2.0 is using a configuration file.
pquery v3.x accept run options from configuration file only.

pquery v3.x is under active development. v2.x is production, v1.0 is no longer in use.

# What is new in pquery v3.x?
pquery v3.x can be used for single and multi-node (cluster, replication etc.) testing. Major change in v3.x is PostgreSQL support. It can send *different* SQL to each tested node and you can PostgreSQL and MySQL tests in the same time. It is also possible to enable the SQL randomizer only for particular nodes.

One can also specify if a pquery worker should be started for a given node by setting ```run = YES | NO``` option for such a node in the configuration file.

# What is the pquery framework?
When the pquery binary is used in combination with the Bash scripted pquery framework and a medium spec QA server (Intel i7/16GB/SSD), a QA engineer can achieve 80+ mysqld crashes per hour. The pquery framework further offers automatic testcase creation, bug filtering, sporadic issue handling, true multi-threaded testcase reduction, near-100% bug reproducibility and much more. The pquery framework furthermore contains high quality SQL input files, and "already known bug" filter lists for Percona Server and MySQL Server. The pquery framework is also GPLv2 licensed, and available from GitHub here: https://github.com/Percona-QA/percona-qa

# What is reducer.sh?
reducer.sh is a powerful multi-threaded SQL testcase simplification tool. It is included in the pquery Framework (https://github.com/Percona-QA/percona-qa), as https://github.com/Percona-QA/percona-qa/blob/master/reducer.sh. It is developed and maintained by Roel Van de Paar.

# Any pquery success stories?
+ In the first ~2 months of it's life, over 200 bugs were logged with Oracle, Percona and TokuTek, most with high quality short testcases.
+ Early MySQL Server 5.7 versions, including RC1 & RC2, were tested with pquery in preparation for Percona Server 5.7. Many bugs, especially in RC1, were discovered & logged. Chapeau to the [MySQL server team](https://mysqlserverteam.com/) who triaged all bugs & resolved major bugs as can be seen in the [5.7.7](http://mysqlserverteam.com/the-mysql-5-7-7-release-candidate-is-available/) and [5.7.8](http://mysqlserverteam.com/the-mysql-5-7-8-release-candidate-is-available/) MYSQL server team notes.
+ Early MySQL Server 8.0 and Percona Server 8.0 versions were tested with pquery. Many bugs were discovered & logged.
+ Extensive testing of Percona XtraDB Cluster was achieved through a multi-node implementation of pquery. It's features are included in the publically available v2.0. Many bugs were found, logged and fixed. Chapeau to [Ramesh V Sivaraman](https://www.linkedin.com/in/ramesh-v-sivaraman-b0087619/) and [Krunal Bauskar](https://www.linkedin.com/in/krunal-bauskar-b7a0b66/).
+ A significant number of query correctness bugs were discovered in RocksDB

# How to build pquery?
1. Install cmake >= 2.6 and C++11 compiler, probably from GCC >= 4.7 (gcc-c++ for RedHat-based, g++ for Debian-based), the development files for your MySQL version/fork, and potentially OpenSSL and AIO development files and/or other deps if needed.
2. Change dir to pquery
3. Run cmake with no options to have optimised build with MySQL and PostgreSQL support (assuming all dependency are installed). OR, you can run cmake with options below to change the build.
Options are:
  * *CMAKE_BUILD_TYPE* - **Release** by default, other options are **Debug**, **RelWithDebInfo**, **MinSizeRel**. For more informaton see https://cmake.org/cmake/help/v3.0/variable/CMAKE_BUILD_TYPE.html
  * *DEVELOPER_MODE* - **OFF** by default, enables STRICT_FLAGS, WITH_ASAN, CMAKE_VERBOSE_MAKEFILE, disables NATIVE_CPU and setting CMAKE_BUILD_TYPE to *Debug*
  * *WITH_ASAN* - **OFF** by default, enables address sanitizer (for debugging pquery itself), available in GCC >= 4.8
  * *STRICT_FLAGS* - **ON** by default, compile pquery with many compiler warnings enabled
  * *NATIVE_CPU* - **OFF** by default, compile pquery with processor optimization. *OFF* allows running the binary on all types of processors. If this set to *ON*, the binary will be strictly bound to the CPU used at the time of building, and may therefore work only on the machine it was built on. Enable it to favor performance over portability. When enabled, pquery will be built with `-march=native` resulting in all of the registers and capabilities from the currently installed CPU being used.

  * *WITH_MYSQL* - **ON** by default, build pquery with MySQL support (required for any flavour)
  * *WITH_PGSQL* - **ON** by default, build pquery with PostgreSQL support
  * *MYSQL_BASEDIR* - empty by default, specifies the path to MySQL, points cmake to extracted MySQL tarball
  * *PGSQL_BASEDIR* - empty by default, specifies the path to PostgreSQL, points cmake to extracted PostgreSQL tarball

  * *STATIC_SSL*   - **OFF** by default, build pquery with statically linked OpenSSL (probably violates current OpenSSL license)
  * *STATIC_MYSQL* - **OFF** by default, build pquery with statically linked MySQL
  * *STATIC_PGSQL* - **OFF** by default, build pquery with statically linked PostgreSQL

  * *MYSQL_FORK* - **MYSQL** by default, specifies MySQL fork/flavour to build pquery with. Options are *MYSQL*, *MARIADB*, *WEBSCALESQL*, *PERCONASERVER*, *PERCONACLUSTER*

  a note for *STATIC_MYSQL* - For most distributions, the static library is included in standard downloads and definitely should be in place if you build MySQL/Percona Server yourself. However, MariaDB is not providing static client library in the standard optimized package, so pquery will automatically compile with a shared MariaDB library (which has to be installed on the OS first, i.e. yum/apt-get install mariadb-devel). In other words, when using ```-DWITH_MYSQL=ON -DMYSQL_FORK=MARIADB```, *STATIC_MYSQL* is turned off by default.


4. If you have MySQL | Percona Server | WebScaleSQL | MariaDB installed to some custom location you may consider setting the additional flags to cmake: *MYSQL_INCLUDE_DIR* and *MYSQL_LIBRARY*. OR, you can set *MYSQL_BASEDIR* variable if you have binary tarball extracted to some custom place for fully automatic library detection (recommended).

5. (still undecided for v3.x and not yet implemented) The resulting binary will automatically receive an appropriate flavor suffix:
  * *pquery3-ms* for MySQL
  * *pquery3-ps* for Percona Server
  * *pquery3-ws* for WebScaleSQL
  * *pquery3-md* for MariaDB

Please note that only the MySQL client library will be linked statically if STATIC_MYSQL is set, all other required libraries (AIO, SSL, etc) will be linked dynamically.

# Can you give an easy build example using an extracted Percona Server tarball?
```
$ cd pquery
$ ./clean-tree.sh  # Important note: this removes any local updates you may have made
$ cmake . -DWITH_MYSQL=ON -DMYSQL_FORK=PERCONASERVER -DMYSQL_BASEDIR=/tmp/percona-server-5.7.21-20-linux-x86_64
$ make
$ sudo make install # If you want pquery to be installed on the system, otherwise the binary can be found in ./src
$ ./clean-tree.sh  # Ref above
$ ... build your other MySQL flavors/forks here in the same way, modifying the basedir and the servertype (both -D options) ...
```

# pquery packages

There are currently no official RPM/APT etc. packages.

Automatic package creation is currently in alpha phase. You can generate a simple package using CPack:

```
$ cpack -G RPM
CPack: Create package using RPM
CPack: Install projects
CPack: - Run preinstall target for: pquery
CPack: - Install project: pquery
CPack: Create package
CPackRPM: Will use GENERATED spec file: /home/percona/pquery/_CPack_Packages/Linux/RPM/SPECS/pquery-ms.spec
CPack: - package: /home/percona/pquery/pquery-ms-2.0.0-Linux.rpm generated.
$
```

You can use *RPM*, *DEB*, *TGZ*, *STGZ* and other suitable targets for Linux and Unix

# Any known build issues?

There is one known build issue, currently seen only when building using WebScaleSQL. If you see the following;

```
[ 50%] Building CXX object src/CMakeFiles/pquery-ws.dir/pquery.cpp.o
In file included from /home/percona/pquery/src/node.hpp:12:0,
                 from /home/percona/pquery/src/pquery.hpp:6,
                 from /home/percona/pquery/src/pquery.cpp:15:
/<your_basedir>/include/my_global.h:1197:27: fatal error: my_stacktrace.h: No such file or directory
 #include <my_stacktrace.h>
                           ^
compilation terminated.
make[2]: *** [src/CMakeFiles/pquery-ws.dir/pquery.cpp.o] Error 1
make[1]: *** [src/CMakeFiles/pquery-ws.dir/all] Error 2
make: *** [all] Error 2
```

Then simply copy the my_stacktrace.h file from the include directory of your source code copy (i.e. WebScaleSQL's source code) to the basedirectory used, e.g.

  cp /source_code_dir/include/my_stacktrace.h /base_dir/include/

# Any (build-related) runtime issues?

If pquery exits with exit code 4 (use `echo $?` at your command line to see the exit code directly after pquery terminates), or you see any other odd occurences when using pquery, please check dmesg log. If you see things like;

```
[16354204.300555] traps: pquery2-ps[24837] trap invalid opcode ip:42439f sp:7f90197fbe80 error:0 in pquery2-ps[400000+366000]
[16354210.748753] traps: pquery2-ps[25207] trap invalid opcode ip:42439f sp:7fa7cd7fbe80 error:0 in pquery2-ps[400000+366000]
```

You are using a binary compiled binary with strict CPU binding/optimization (ref the `NATIVE_CPU` build flag above) while using it on a (likely older) machine which has a CPU incompatbile with the original build CPU.

To fix this, you can chose from 3 options;

1. Compile pquery locally on this machine with the `-DNATIVE_CPU=ON` cmake flag, which will then automatically have the best speed optimization for this CPU on this machine
2. Compile with the `-DNATIVE_CPU=OFF` cmake flag (the default) and use the resulting binary on any CPU. As described this option may be somewhat slower (perhaps in the area of 2% - unconfirmed)
3. If you want the absolute fastest pquery ever (untested), and you are very experienced with cmake, you can build the binary and "bind" it to the exact CPU you are using. Have a look at https://github.com/tunabrain/tungsten/blob/master/cmake/OptimizeForArchitecture.cmake - this optimization is very strict, and will fail to start on older or other processors.

# What options does pquery accept?

First, take a quick look at ``` pquery -h``` to see available modes and options.

# v3.x Command line options example:
```-h``` - help with config example
```-c``` - config to run with
```-v``` - version with commit info

# v3.x Configuration file example:
```
# Section for master process
[master]
# Directory to store logs
logdir = /tmp
# Logfile for master process
logfile = pquery3-master.log

[mysql.ci.percona.com]
# The database to connect to
database = mytestdb
# Database type, may be MySQL OR PostgreSQL
dbtype = mysql
# IP address to connect to, default is AF_UNIX
address = 192.168.10.1
# The port to connect to
port = 3306
# The SQL input file
infile = pquery-mysql.sql
# Directory to store logs
logdir = /tmp
# Socket file to use
socket = /tmp/my.sock
# The DB userID to be used
user = pquery
# The DB user's password
password = pquery123!
# The number of threads to use by worker
threads = 10
# The number of queries per thread
queries-per-thread = 100
# Log all queries
log-all-queries = No
# Log succeeded queries
log-succeeded-queries = No
# Log failed queries
log-failed-queries = No
# Execute SQL randomly
shuffle = Yes
# Extended output of query result
log-query-statistics = No
# Log query duration in milliseconds
log-query-duration = No
# Log output from executed query (separate log)
log-client-output = No
# Log query numbers along the query results and statistics
log-query-number = No

[postgres.ci.percona.com]
address = 10.10.6.10
dbtype = postgres
infile = pquery-pgsql.sql
# default for "run" is No, need to set it explicitly to YES if needed
run = No

```

Note that logfiles (including SQL log files) are now overwritten.
If SQL logs are appended to in old v2.0 versions, it will reduce issue reproducibility. To avoid this, simply use a new log file for each pquery run. The [pquery framework](https://github.com/Percona-QA/percona-qa) (ref [pquery-run.sh](https://github.com/Percona-QA/percona-qa/blob/master/pquery-run.sh)) already takes care of this automatically.

# Where can I find more information on pquery?
+ [The future of MySQL quality assurance: Introducing pquery](https://www.percona.com/blog/2015/02/04/future-mysql-quality-assurance-introducing-pquery/)
+ [pquery binaries with statically included client libs now available!](https://www.percona.com/blog/2015/04/09/pquery-binaries-with-statically-included-client-libs-now-available/)
+ [MySQL QA Episode 4: QA Framework Setup Time!](https://www.percona.com/blog/2015/07/08/mysql-qa-episode-4-qa-framework-setup-time/)
+ [MySQL QA Episode 5: Preparing Your QA Run with pquery](https://www.percona.com/blog/2015/07/13/mysql-qa-episode-5-preparing-your-qa-run-with-pquery/)

# Where can I find more information on the pquery Framework?
+ [13 Part video tutorial on MySQL QA, pquery, the pquery Framework, Bash scripting & more](https://www.youtube.com/playlist?list=PLWhC0zeznqkkgBcV3Kn-eWhwJsqmp4z0I "13 Part video tutorial in HD quality on MySQL QA, pquery, the pquery Framework, Bash scripting, GDB & more, presented by Roel Van de Paar")
+ [Blog post on this 13 Part QA Series video tutorials](https://www.percona.com/blog/2015/03/17/free-mysql-qa-and-bash-linux-training-series/ "13 Part video tutorial in HD quality on MySQL QA, pquery, the pquery Framework, Bash scripting, GDB & more, presented by Roel Van de Paar")

# Where can I find more information on reducer.sh?
+ [Reducer.sh – A powerful MySQL test-case simplification/reducer tool](https://www.percona.com/blog/2014/09/03/reducer-sh-a-powerful-mysql-test-case-simplificationreducer-tool/)
+ [MySQL QA Episode 7: Reducing Testcases for Beginners – single-threaded reducer.sh!](https://www.percona.com/blog/2015/07/21/mysql-qa-episode-7-single-threaded-reducer-sh-reducing-testcases-for-beginners)
+ [MySQL QA Episode 8: Reducing Testcases for Engineers: tuning reducer.sh](https://www.percona.com/blog/2015/07/23/mysql-qa-episode-8-reducing-testcases-engineers-tuning-reducer-sh/)
+ [MySQL QA Episode 9: Reducing Testcases for Experts: multi-threaded reducer.sh](https://www.percona.com/blog/2015/07/28/mysql-qa-episode-9-reducing-testcases-experts-multi-threaded-reducer-sh/)
+ [MySQL QA Episode 10: Reproducing and Simplifying: How to get it Right](https://www.percona.com/blog/2015/07/31/mysql-qa-episode-10-reproducing-simplifying-get-right/)

# Contributors
* Alexey Bychko - C++ code, cmake extensions
* Roel Van de Paar - invention, scripted framework
* For the full list of contributors, please see [CONTRIBUTORS](https://github.com/Percona-QA/pquery/blob/master/doc/CONTRIBUTORS)
