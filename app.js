const fs = require('fs');
path = require('path');
const express = require('express');
const fileUpload = require('express-fileupload');
const app = express();
const createWav = require('./encode-wav');
const stt = require('./stt');

app.use('/', express.static(path.join(__dirname, 'public')));
app.use(fileUpload());

app.post('/upload', function(req, res) {
    if (!req.files)
        return res.status(400).send('No files were uploaded.');
    console.log("No. of files received: ", Object.keys(req.files).length);
    Object.keys(req.files).forEach(function(key) {
        console.log("Received File: ", req.files[key].name, new Date());
        req.files[key].mv('./public/uploads/' + req.files[key].name, function(err) {
            if (err) return res.status(500).send(err);
            createWav(req.files[key].name);
            stt(req.files[key].name).then(

            )
        });
    });
    res.send('File(s) uploaded!');
});

app.get('/getReportData', function(req, res) {
    var resp = [];
    fs.readdir('./public/wav', (err, files) => {
        files.forEach(file => {
            if (file.indexOf('wav') > -1) {
                var d = file.substring(0, file.indexOf(".")).split("_");
                try {
                    txt = fs.readFileSync("./public/wav/" + file.substring(0, file.indexOf(".")) + ".txt", "utf8")
                } catch (e) {
                    txt = ""
                }
                resp.push({ "date": new Date(Number(d[0]), Number(d[1]), Number(d[2]), Number(d[3]), Number(d[4]), Number(d[5])).getTime(), "wav": file, "text": txt ? txt : "" });
            };

            //resp.reverse();

        })
        resp.sort(function(a, b) { return b.date - a.date });
        res.send(resp);
    });
});

app.get('/deleteWAV', function(req, res) {
    var filetodel = req.query.wav;
    console.log("Deleting", filetodel);
    fs.unlinkSync("./public/wav/" + filetodel);
    res.send("File " + filetodel + " deleted!");
});


app.listen(3000, function() {
    console.log("Server listening on port 3000");
});