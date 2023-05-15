gross - Greylisting of suspicious sources

Introduction
------------

Gross is a greylisting server. It is designed to be fast and resource
efficient. It can be configured to perform a number of checks, and enforce
greylisting only for hosts that have not passed them.  Hosts that failed to
pass multiple checks can be blocked immediately.

Gross can be replicated and run parallel on two servers.

It supports Postfix, Exim, Sendmail and Oracle Communications Messaging Server.

Authors
-------

Gross was originally written by Eino Tuominen <eino@utu.fi>
and Antti Siira <antti@utu.fi>

Since 2023 maintained by Dmitry Mikhirev <dmitry@mikhirev.ru>

Design and Operation
--------------------

Gross consists of grossd, the greylisting daemon, and
a client plugin for OCMS. The server also implements
[Postfix policy delegation protocol][postfix] and Milter protocol.

Upon receiving a request from a client, grossd first
validates it. The request includes a triplet (smtp-client-ip,
sender-address, recipient-address). A hash is then
calculated and matched against the Bloom filter. If a
match is found, server sends an OK message.

If the triplet has not been seen before, grossd then
performs configured checks. Supproted checks are querying
DNSBL databases for the smtp-client-ip, checking canonical
hostname and matching HELO name to IP address.
If something suspicious is found, grossd sends client a
GREY response. If DNSBL tests are negative, an OK response
is sent. Database is then updated; currently there are
two updating styles: either to always update or to only
update if the result was GREY.

As mentioned above, the data store is implemented with
[Bloom filters][bloom]. A Bloom filter is a very efficient way
to store data. It's a probabilistic data structure,
which means that there is a possibility of error when querying
the database. False positives are possible, but false
negatives are not. This means that there is a possibility
that grossd will falsely give an OK response when a
connection should be greylisted. By sizing the bloom
filters, you can control the error possibility to meet your
needs. The right bloom filter size depends on the number
of entries in the database, that is, the retention time
versus the number of handled connections.

DNS queries are done asynchronously using [c-ares] library

Requirements
------------

You'll need cmake and a decent compiler, such as gcc or clang.
To build manual pages, asciidoctor is needed.

In order to configure dnsbl queries, reverse DNS lookup and HELO check, you
need c-ares library for asynhronous dns queries. You can download it
[here][c-ares]. To enable SPF check, you need [libspf2].

Milter protocol support requres libmilter that is a part of
[sendmail].

Compiling
---------

You should be able to compile it for testing by just commanding:

```
$ mkdir build
$ cd build
$ cmake ..
$ make
$ sudo make install
```

Default install location is /usr/local which can be overridden by
specifying a `-DCMAKE_INSTALL_PREFIFIX=<path>` argument to cmake.
Run `cmake -LH ..` or use either `cmake-gui` or `ccmake` to see
other options.

Basic Configuration
-------------------

An example configuration is included in `doc/example/grossd.conf`.
It is installed also as the default configuration in
$PREFIX/etc/grossd.conf. You may tell grossd to use another config
file via -f command line option.

Grossd query server defaults to port 5525/tcp with `proto=postfix`,
port 5525/udp with `proto=sjsms` and replication service
(if configured) to port 5526/tcp. Grossd listens for status queries
on port 5522/tcp.

Detailed information about each option is included within the
example configuration file.

Bloom Filter Configuration
--------------------------

The server keeps the state information in the Bloom filter bit arrays.
By the nature of the filters you can insert elements in the filters, but
not to remove them. So we actually use several filters, and rotate them
on intervals. The actual filter is just a logical OR of those rotating
filters. `number_buffers` is the number of the rotating filters, and
`rotate_interval` is the interval in seconds.

The default setting in the example grossd.cnf is:

```
number_buffers = 8
rotate_interval = 3600
```

So, this will give you approximately 60 * 7.5 minutes worth of state
information. That is, any greylisted triplet will be stored for
7.5 hours on average.  If you find that remote servers are taking
longer than 7.5 hours to retry their messages, then you should increase
the the `number_buffers` setting accordingly.  Alternatively, you could
increase the `rotate_interval`, but setting this value too high isn't
recommended.

The `filter_bits` setting (default is 24) defines the size of each
Bloom filter.  Increasing the `filter_bits` setting will reduce the
likelihood of false positives within the filter.  See the
[Wikipedia article][bloom] for more information.

Create Statefile
----------------

If you get this far, you can start the server. I suggest you
run the server first in foreground with -d option. If you
configured statefile, you have to start grossd with -C option
in order to create the state file.

```
$ sudo -u nobody $PREFIX/sbin/grossd -C
```

Note: make sure that the run-time user has permissions to create
and write to the statefile.

Server Startup
--------------

To start the grossd server, just run this.

```
$ sudo -u nobody $PREFIX/sbin/grossd
```

After starting, you can test it with $PREFIX/bin/gclient.

MTA Configuration
-----------------

Gross works with Postfix's native filtering protocol.

On the other hand, SJSMS needs some mapping entry for queries.
This is an example production server config:

```
ORIG_MAIL_ACCESS

! allow all DSNs and MDNs
  TCP|*|*|*|*|*|*|tcp_local||*|*  $Y$E
! allow all incoming mail to postmaster and abuse
  TCP|*|*|*|*|*|*|tcp_local|*|*|postmaster@*  $Y$E
  TCP|*|*|*|*|*|*|tcp_local|*|*|abuse@*  $Y$E
! use gross to check all triplets (client_ip,sender,recipient)
  TCP|*|*|*|*|SMTP/*|*|tcp_local|*|*|*  $[/usr/local/gross/lib/grosscheck.so,grosscheck,10.10.13.1,10.10.13.2,5525,$2,$=$8$_,$=$6$_,$=$4$_]
```

The server will always query the first server if it's available.

First, when you have only one server, you can test it like this:

```
  TCP|*|*|*|*|SMTP/*|*|tcp_local|*|*|*  $[/path/to/grosscheck.so,grosscheck,127.0.0.1,,5525,$2,$=$8$_,$=$6$_,$=$4$_]
```

Sophos Blocker Integration
--------------------------

grossd is able to query the proprietary Sophos Blocker service, which
is basically a private DNSBL that is part of the Sophos PureMessage
product.  You can configure grossd to query your existing
PureMessage system, or install it on the same server(s) as gross.

Known Issues
------------

See https://codeberg.org/bizdelnick/gross/issues for bugs
and known issues.

Logging and Troubleshooting
---------------------------

grossd logs everything through syslog. Facility is `LOG_MAIL`, loglevel
defaults to `INFO`. You can control logging with `log_level` and
`syslog_facility` configuration options.

You can enable full debugging by starting grossd with -D command line
option. You may also want -d, as grossd then writes output on standard
terminal instead of syslog.

gclient is a tool for manually querying the grossd server.  You can
install and run gclient on any server.

```
$ gclient PROTOCOL sender recipient ip_address [runs] [host port]
$ /usr/local/bin/gclient sjsms foo@abc.com bar@def.org 127.0.0.2 2 10.10.10.13 5525
```

(note that 127.0.0.2 will always be blacklisted)

Development and Licensing
-------------------------

Gross is published on a BSD-like license.

See https://codeberg.org/bizdelnick/gross for more information.

If you have any suggestions regarding the software or wish
to take part on the development, feel free to contact the authors.
We'll be happy to hear if you dare to try out the software.

[postfix]: https://www.postfix.org/SMTPD_POLICY_README.html
[c-ares]: https://c-ares.org/
[libspf2]: https://www.libspf2.org/
[sendmail]: https://ftp.sendmail.org/
[bloom]: https://en.wikipedia.org/wiki/Bloom_filter
