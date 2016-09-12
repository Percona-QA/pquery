# What is pquery?
pquery is an open-source (GPLv2 licensed) multi-threaded test program created to stress test the MySQL server (in any flavor), either randomly or sequentially, for QA purposes. Given it's modern C++ core, it is able to maximise the physical server's queries per second (qps) rate. pquery is an acronym for 'parallel query'. Prebuild pquery binaries (with statically linked client libraries) for Percona Server, MySQL Server, MariaDB, and WebScaleSQL are available as part of the pquery framework.

+ *pquery v1.0* was designed for single-node MySQL setup, and accepts command line options only. Ref ```pquery --cli-help```
+ *pquery v2.0* was designed for multi-node MySQL setups, and accepts command line options as well as options from a configuration file in INI format. Ref ```pquery --config-help```

Please note that v2.0 accepts the same CLI options as v1.0 does, for backwards compatibility. And, alike to v1.0, it can handle a single node setup in that mode. The recommended way to pass all options and params to pquery v2.0 is using a configuration file.

pquery v2.0 is under active development, and v1.0 will be supported by request.

# What is new in pquery v2.0?
pquery v2.0 can be used for single and multi-node (cluster, replication etc.) testing. It can sent *different* SQL to each tested node. It is also possible to enable the SQL randomizer only for particular nodes. It also supports the same features, and is largely backwards compatible with v1.0 (some output file names and locations have changed).

One can now also specify if a pquery worker should be started for a given node by setting ```run = YES | NO``` option for such a node in the configuration file.

# What is the pquery framework?
When the pquery binary is used in combination with the Bash scripted pquery framework and a medium spec QA server (Intel i7/16GB/SSD), a QA engineer can achieve 80+ mysqld crashes per hour. The pquery framework further offers automatic testcase creation, bug filtering, sporadic issue handling, true multi-threaded testcase reduction, near-100% bug reproducibility and much more. The pquery framework furthermore contains high quality SQL input files, and "already known bug" filter lists for Percona Server and MySQL Server. The pquery framework is also GPLv2 licensed, and available from GitHub here: https://github.com/Percona-QA/percona-qa

# What is reducer.sh?
Reducer.sh is a powerful multi-threaded SQL testcase simplification tool. It is included in the pquery Framework (https://github.com/Percona-QA/percona-qa), as https://github.com/Percona-QA/percona-qa/blob/master/reducer.sh It is developed and maintained by Roel Van de Paar.

# Any pquery success stories?
+ In the first ~2 months of it's life, over 200 bugs were logged with Oracle, Percona and TokuTek, most with high quality short testcases.
+ Early MySQL Server 5.7 versions, including RC1 & RC2, were tested with pquery in preparation for Percona Server 5.7. Many bugs, especially in RC1, were found & logged. Chapeau to the MySQL server team who triaged all bugs & resolved major bugs as can be seen in the [5.7.7](http://mysqlserverteam.com/the-mysql-5-7-7-release-candidate-is-available/) and [5.7.8](http://mysqlserverteam.com/the-mysql-5-7-8-release-candidate-is-available/) MYSQL server team notes.
+ A significant number of query correctness bugs were discovered in RocksDB

# How to build pquery?
1. Install cmake >= 2.6 and C++ compiler >= 4.7 (gcc-c++ for RedHat-based, g++ for Debian-based), the development files for your MySQL version/fork, and potentially OpenSSL and AIO development files and/or other deps if needed.
2. Change dir to pquery
3. Run cmake with the required options, which are:
  * *PERCONASERVER* - **OFF** by default, build pquery with Percona Server support
  * *WEBSCALESQL* - **OFF** by default, build pquery with WebScaleSQL support
  * *MYSQL* - **OFF** by default, build pquery with Oracle MySQL support
  * *MARIADB* - **OFF** by default, build pquery with MariaDB support
  * *STATIC_LIB* - **ON** by default, compile pquery with MySQL | Percona Server | WebScaleSQL static client library instead of dynamic
  * *DEBUG* - **OFF** by default, compile pquery with debug inforamation for GDB
  * *STRICT* - **ON** by default, compile pquery with strict flags
  * *ASAN* - **OFF** by default, address sanitizer, available in GCC >= 4.8
4. If you have MySQL | Percona Server | WebScaleSQL | MariaDB installed to some custom location you may consider setting the additional flags to cmake: *MYSQL_INCLUDE_DIR* and *MYSQL_LIBRARY*. OR, you can set *BASEDIR* variable if you have binary tarball extracted to some custom place for fully automatic library detection (recommended).
5. The resulting binary will automatically receive an appropriate flavor suffix:
  * *pquery-ms* for MySQL
  * *pquery-ps* for Percona Server
  * *pquery-ws* for WebScaleSQL
  * *pquery-md* for MariaDB

Please note that only the MySQL client library will be linked statically if STATIC_LIB is set, all other required libraries (AIO, SSL, etc) will be linked dynamically.

# Can you give an easy build example using an extracted Percona Server tarball?
```
$ cd pquery
$ ./clean-tree.sh  # Important note: this removes any local updates you may have made
$ cmake . -DPERCONASERVER=ON -DBASEDIR=/tmp/Percona-Server-5.6.26-rel73.2-Linux.x86_64
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

# Any known issues?

There is one known build issue, currently seen only when building using WebScaleSQL. If you see the following;

```
[ 50%] Building CXX object src/CMakeFiles/pquery-ws.dir/pquery.cpp.o
In file included from /home/roel/pquery/src/node.hpp:12:0,
                 from /home/roel/pquery/src/pquery.hpp:6,
                 from /home/roel/pquery/src/pquery.cpp:15:
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

# What options does pquery accept?

First, take a quick look at ``` pquery --help, pquery --config-help, pquery --cli-help ``` to see available modes and options.

# v1.0/2.0 Command line options example:

Option | Function| Example
--- | --- | ---
--database | The database to connect | --database=test
--address | IP address to connect to | --address=127.0.0.1
--port | The port to connect to | --port=3306
--infile | The SQL input file | --infile=./main-ms-ps-md.sql
--logdir | Log directory | --logdir=/tmp/123
--socket | Socket file to use | --socket=/tmp/socket.sock
--user | The MySQL userID to be used | --user=root
--password | The MySQL user's password | --password=pazsw0rd
--threads | The number of client threads to use | --threads=1
--queries-per-thread | The number of queries to randomly execute per thread | --queries-per-thread=100000
--verbose | Duplicates log to console when threads=1 | --verbose
--log-all-queries | Log all queries yes/no | --log-all-queries
--log-succeeded-queries | Log successful queries yes/no | --log-succeeded-queries
--log-failed-queries | Log failed queries yes/no | --log-failed-queries
--no-shuffle | Replay SQL shuffled (randomly) or not (sequentially) | --no-shuffle
--log-query-statistics | Extended output of query result | --log-query-statistics
--log-query-duration | Log query duration in milliseconds | --log-query-duration
--test-connection | Test connection to server and exit | --test-connection
--log-query-numbers | Write query numbers to log | --log-query-numbers
--log-client-output | Log query output to separate file | --log-client-output

# v2.0 Configuration file example:
```
[node0.ci.percona.com]
address = 192.168.10.1
user = test
password = test
database = test
# relative or absolute path so sql file
infile = pquery.sql
verbose = True
threads = 10
queries-per-thread = 100
run = Yes
# Log all queries
log-all-queries = Yes
# Log successful queries
log-succeeded-queries = No
# Log failed queries
log-failed-queries = Yes
# Execute SQL randomly
shuffle = Yes
# Extended output of query result
log-query-statistics = Yes
# Log query duration in milliseconds
log-query-duration = Yes
# Log query output to separate file
log-client-output = No
# Write also query # from SQL file (to compare query and output for example)
log-query-numbers = No

[node1.ci.percona.com]
address = 127.0.0.1
user = test
password = test
infile = pquery.sql
shuffle = Yes
queries-per-thread = 150
run = No

[node2.ci.percona.com]
address = 127.0.0.1
user = root
password = 1q2w3e
infile = pquery2.sql
run = No
```

Note that logfiles (including SQL log files) are appended to, not overwritten. If SQL logs are appended to, it will reduce issue reproducibility. To avoid this, simply use a new log file for each pquery run. The [pquery framework](https://github.com/Percona-QA/percona-qa) (ref [pquery-run.sh](https://github.com/Percona-QA/percona-qa/blob/master/pquery-run.sh)) already takes care of this automatically.

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
