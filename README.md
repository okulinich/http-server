Config-file explanation:
* configuration lines can be placed in random order
* if configuration lines are not specified, specified incorrect,
  or file is absent - server will provide default his config
* port may be specified as 80 or from 1024 to 65535
* root folder MUST exist and cannot be outside of the server 
  working directory, the same about default document, if they
  are absent - server will run, but you would probably get a 
  404 error all the time, if path to file or root folder goes
  somewhere outside the working directory - server will provide
  his default config.
* ip version can be equal to 4 (IPv4), 6 (IPv6) or 0 (both IPv4
  and IPv6)
* log - enables ("TRUE" or "1") and disables(any string not equal
  to "TRUE" or "1") logging
* if interface is not "none" or "0", it will be used to bind listening
  socket
* backlog - the maximum length of pending connections queue, MUST be > 0
