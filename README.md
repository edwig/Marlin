Introduction and purpose
========================

This is the Marlin webserver library. Version 7.4.0

The �Marlin' components � webserver and web client � are built in C++ around a number 
of general classes that take care of optimal performance and have the purpose that 
the server and client parts can be plugged-in in a C++ project.
The main reason to build this library was to expand an existing web server with 
basic HTTPS capabilities like SSL and TLS connections and advanced authentication 
possibilities like digest- and Kerberos authentication. Instead of building all 
those components again, I choose to use the general existing Microsoft 
components �HTTP-Server API 2.0� and �WinHTTP API 5.1�

Reason to use C++ for these components � and no .NET technology � has to do with 
the performance of the components in comparison to the WCF services. 
Typically these compo-nents are a many times faster than a WCF implementation in Dot NET. 
Certainly if you configure all kinds of W3C standards like signing of messages, 
security encryption and  reliable-messaging.
Another reason was the need to be able to run a webservice on remote desktops 
(Citrix !) environments. This was clearly not going to happen with the IIS ISAPI framework. 
Although IIS 7.0 and higher has done much good with the new integrated pipelining of the 
requests, enabling an IIS server on every desktop was not an option. 

![Marlin](/Documentation/Marlin.png)

The name
--------
The white marlin is a common type of marlin in the Atlantic ocean. 
This is about the fastest fish in that ocean and it�s a very secretive fish. 
If you manage to catch one on a fishing line, it will struggle on � unseen � in the deep. 
Like a fast webserver would.
