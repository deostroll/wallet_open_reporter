var fs = require('fs');

var buffer = fs.readFileSync('./uploads/a-0.dat');

var maxValue = Math.max.apply(Math, buffer);

console.log(maxValue);
