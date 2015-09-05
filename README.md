# PQuery
PQuery is a small utility written to test/crash MySQL for QA purposes. It's name stands for P(arallel) Query.

# Build HOWTO
1. install cmake >= 2.6 and GCC >= 4.7 and dependencies, usually OpenSSL and AIO development files, other deps if needed. 
2. change dir to pquery
3. run cmake with required options, which are:
* *WEBSCALESQL* - **OFF** by default, build PQuery with WebScaleSQL support
* *PERCONASERVER* - **OFF** by default, build PQuery with Percona Server support 
* *MYSQL* - **OFF** by default, build PQuery with Oracle MySQL support 
* *STATIC_LIB* - **OFF** by default, compile PQuery with MySQL | Percona Server | WebScaleSQL static client library instead of dynamic
* *DEBUG* - **OFF** by default, compile PQuery with debug inforamation for GDB
* *STRICT* - **ON** by default, compile PQuery with strict flags   
4. if you have MySQL | Percona Server | WebScaleSQL installed to some custom location you may consider setting the additional flags to cmake:
* *MYSQL_INCLUDE_DIR*
* *MYSQL_LIBRARY*
5. resulting binary will have appropriate extension:
* *pquery-ms* for MySQL
* *pquery-ps* for Percona Server
* *pquery-ws* for WebScaleSQL

# Example
so, your commands may look like the following example:
```
$ cd pquery
$ cmake . -DPERCONASERVER=ON -DSTATIC_LIB=ON 
$ make
$ ./src/pquery-ps
```
also, you can compile 2 or 3 PQuery variants, just run cmake with different options from different directories or use ```git clean -xfd``` after compilation

# PQuery accepts the following options
* --database
* --address 
* --infile
* --logdir
* --socket
* --port
* --user
* --password
* --threads
* --queries_per_thread
* --verbose
* --log_all_queries
* --log_failed_queries
* --no-shuffle

# Links
* https://www.percona.com/blog/2015/02/04/future-mysql-quality-assurance-introducing-pquery/
* https://www.percona.com/blog/2015/07/08/mysql-qa-episode-4-qa-framework-setup-time/
* https://www.percona.com/blog/2015/07/13/mysql-qa-episode-5-preparing-your-qa-run-with-pquery/

# Contributors
* Alexey Bychko - cmake, c++ code
* Roel Van de Paar - PQuery scripted framework
* for full contributors list please see doc/README
