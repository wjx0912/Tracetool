
// ===============================================
// Tracetool node js samples.
// Don't forget to start the viewer first !!!
// ===============================================

"use strict";

const ttrace = require('tracetool');    // default host is 127.0.0.1:81
ttrace.host = "127.0.0.1:81";
ttrace.options.objectTreeDepth = 5;

// clear all traces on the viewer (main traces)
ttrace.clearAll();

// the more simple way to send traces
ttrace.debug.send  ("Simple debug trace");
ttrace.warning.send("Simple warning trace");
ttrace.error.send  ("Simple error trace");

// 2 columns traces
ttrace.debug.send("trace left","trace right");

// indent / unindent
ttrace.debug.indent("level1 using indent method", undefined, undefined, true);   
ttrace.debug.send("level2");
ttrace.debug.send("level2");
ttrace.debug.send("level2");
ttrace.debug.unIndent("end level1", undefined, undefined, true);    

// subnode 
var Node = ttrace.debug.send("levelA using sub node method");
Node.send("levelB as child from levelA");
Node.send("levelB as child from levelA");
Node.send("levelB as child from levelA");

ttrace.debug
    .send("level-I")
       .send("level-II")
           .send("level-III");

// enter method
//...

// call stack
ttrace.debug.sendCaller ("caller");
ttrace.debug.sendStack("stack");


// http://127.0.0.1:3000


const http = require('http');
const hostname = '127.0.0.1';
const port = 3000;

const server = http.createServer((req, res) =>
{
  ttrace.debug.send("Page is requested");

  res.statusCode = 200;
  res.setHeader('Content-Type', 'text/plain');
  res.end('Hello World\n');
});


server.listen(port, hostname, () =>
{
    ttrace.debug.send(`Server running at http://${hostname}:${port}/`);
    console.log(`Server running at http://${hostname}:${port}/`);
});
