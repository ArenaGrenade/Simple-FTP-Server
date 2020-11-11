# Simple-FTP-Server

A simple file transfer server and client implemented using C++ and sockets.
In this implementation we assume that the server exists just to serve a single client and they are both completely dependent on each other for existence.

## The AGServer

This is the server implementation that would be responsible for opening up a socket conenction and serving the connection that it comes across. Error handling is done in the way that, the a `SIGPIPE` is generated whenever a failure in the socket happens, which would then be elevated to a `SIGINT`, which would eventually kill the server process.

## The Client

In this client application, I have implemnted the following commands:

### help

> Returns the help message regarding commands live from the console.

### ls

> Returns a list of files present on the server, available for transfer.

### get file1 file2 ...

> This command is the crux of the whole FTP server-client system. A `get` on the client would fetch the files from the server and then save it to the client folder. There are very many errors possible here and hence a lot of `ack` based error handling has been done.

### quit or exit

> Either of these commands would directly quit both the server and the client.

## The story about `utils.cpp` file

This file might be a bit confusing as it is used primarily inside both the server and client code.

### What it was initially?

This file has 2 basic functions of which there are 2 overloaded kinds each. One is a function that sends a message, either string or integer, and another pair of overloaded functions that recieve a message, which again would a string or a integer. These two functions take away the need to code up individual `send` and `recv` statements for performing an `ack`. These functions automatically send the message (and also the size of a message, considering the message is a string) and wait for an `ack`.

These 2 pairs of overloaded functions, constitute a wrapper around the basic `send` and `recv` unix socket commands, which makes it easier to perform `ack`s and ensure correct buffer sizes in the case of a string.

### Need for the `hand_over` operation

During the code up of this system, one thing seemed very clear - if one wants to run the whole code without any blockups or hold ups on either side, there needs to be an alternation of a low-level `send` and `recv` commands. But, normally the above two command implementation could never be turned around by itself. So, I introduced a new function called `hand_over`, which is basically responsible for giving away sending rights to the other and recieve it.

### Speculations

By all of this one thing would be clear from the `utils.cpp`. There is a notion of data trasfer direction and if any one in the party of the transfer wants to take control of this, it would have to send a `hand_over` request which the other memeber would also accept. This way the data transfer direction would turn around and now the memeber who requested control can send messages to the other. This gives a memeber in the party the credibility and responsibility for any errors it might cause in the transaction.

## The additional script files

## generator.sh

Run as `./generator.sh file 10`.
This creates a file named `file` inside the server directory, filled with `10MB` of random characters.

## testdiff.sh

Run as `./testdiff.sh file`
Considering that a file named `file` has been transferred and its correctness is to be tested. This would automatically check for the similarity of the original files in `server` directory and the one in `client` using the linux `diff` command.
