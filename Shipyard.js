var pjson = require('./package.json');

// External requires
const path = require('path');
var express = require('express');
var bodyParser = require('body-parser');
var jwt = require('jwt-simple');
var _ = require('lodash');
var app = express();
var winston = require('winston');
var moment = require('moment');
var fs = require('fs');

//{ error: 0, warn: 1, info: 2, verbose: 3, debug: 4, silly: 5 }
winston.level = 'debug';

var nodes = [
  {nodeId:'2000', version:'2.4'},
  {nodeId:'2001', version:'1.5'}
];

// Application settings
var isWin = /^win/.test(process.platform);
if (isWin){
  var params = require('../Gravitation/Windows');
}
else{
  var params = require('../Gravitation/Linux');
}

// Local requires
var dbInit = require('./Config/DbInit');
var client;

//*******************
// 1. Parse forms & JSON in body
//*******************
app.use(bodyParser.urlencoded({
    extended: true
}));
app.use(bodyParser.json());

app.get('/status', function (req, res) {
  res.json(
    {
      status: 'online',
      application: pjson.name,
      version: pjson.version,
      description: pjson.description
    }
  );
});

app.get('/firmware/:nodeId*', function(req, res){
  // Find the current firmware version for the node
  for(node in nodes){
    // Determine if this is the requested node
    if(nodes[node].nodeId === req.params.nodeId){
      // Print an debug message
      winston.debug("Node " + nodes[node].nodeId + " : " + nodes[node].version);

      // Send the firmware version to the node
      return res.status(200).send(nodes[node].version);
    }
  }
  // Send version 1.0 out for all unknown nodes.
  res.status(200).send("1.0");
});

app.get('/download/:nodeId/:fw_version/:payload*', function(req, res){
  var ip = req.headers['x-forwarded-for'] || req.connection.remoteAddress;
  winston.debug("New node " + req.params.nodeId + " request firmware from " + ip + " : " + req.params.fw_version + " : " + req.params.payload);

  var file = __dirname + "/Satelites/_" + req.params.nodeId + "/_" + req.params.nodeId + ".ino.generic.bin";
  if (fs.existsSync(file)) {
    res.download(file); // Set disposition and send it.
  } else {
    res.status(404).send("file : " + file + " does not exist");
  }
});

app.get('/download/:nodeId', function(req, res){
  var ip = req.headers['x-forwarded-for'] || req.connection.remoteAddress;
  winston.debug("Old node " + req.params.nodeId + " request firmware from " + ip);

  var file = __dirname + "/Satelites/_" + req.params.nodeId + "/_" + req.params.nodeId + ".ino.generic.bin";
  if (fs.existsSync(file)) {
    res.download(file); // Set disposition and send it.
  } else {
    res.status(404).send("file : " + file + " does not exist");
  }
});

//****************
// 2. Stel middleware in voor serveren van statische bestanden (HTML, CSS, images)
//****************

// routes ======================================================================
function initialize(){
  console.log('Boot Home automation server :: ' + pjson.name + ' :: ' + pjson.version);
  // Activate website
  app.listen(params.application_port.shipyard, function () {
      console.log('Server gestart op poort ' + params.application_port.shipyard);
  });

  winston.info("System started");
};

initialize();
