var fs = require('fs');

var bitrate = 10000;
var bitspersample = 16;
var channels = 1;

function createWAV(name) {
    var wavheader = [];

    wavheader[0] = 'R';
    wavheader[1] = 'I';
    wavheader[2] = 'F';
    wavheader[3] = 'F';
    //wavheader[4] to wavheader[7] size of data + header -8
    wavheader[8] = 'W';
    wavheader[9] = 'A';
    wavheader[10] = 'V';
    wavheader[11] = 'E';
    wavheader[12] = 'f';
    wavheader[13] = 'm';
    wavheader[14] = 't';
    wavheader[15] = ' ';
    wavheader[16] = 16;
    wavheader[17] = 0;
    wavheader[18] = 0;
    wavheader[19] = 0;
    wavheader[20] = 1;
    wavheader[21] = 0;
    wavheader[22] = 1;
    wavheader[23] = 0;
    // wavheader[24] to wavheader[27] samplerate hz
    // wavheader[28] to wavheader[31] samplerate*1*1
    // optional bytes can be added here
    wavheader[32] = bitspersample * channels / 8;
    wavheader[33] = 0;
    wavheader[34] = bitspersample;
    wavheader[35] = 0;
    wavheader[36] = 'd';
    wavheader[37] = 'a';
    wavheader[38] = 't';
    wavheader[39] = 'a';
    //wavheader[40] to wavheader[43] sample number

    // save raw audio into unsigned 8bit wav
    // void headmod(long value, byte location){
    // // write four bytes for a long
    // tempfile.seek(location); // find the location in the file
    // byte header[4];
    // header[0] = value & 0xFF; // lo byte
    // header[1] = (value >> 8) & 0xFF;
    // header[2] = (value >> 16) & 0xFF;
    // header[3] = (value >> 24) & 0xFF; // hi byte
    // tempfile.write(header,4); // write the 4 byte buffer
    // }

    function headmod(value, location, header) {
        // write four bytes for a long
        // tempfile.seek(location); // find the location in the file
        header[location + 0] = value & 0xFF; // lo byte
        header[location + 1] = (value >> 8) & 0xFF;
        header[location + 2] = (value >> 16) & 0xFF;
        header[location + 3] = (value >> 24) & 0xFF; // hi byte
        // tempfile.write(header,4); // write the 4 byte buffer

    }

    var databuf = fs.readFileSync('./public/uploads/' + name);
    var buf = Buffer.allocUnsafe(2 * databuf.length);
    console.log("buffer size:", buf.length);
    // converting unsigned 8bit to signed 16bit wav
    for (var i = 0; i < databuf.length; i++) {
        var value = databuf[i];
        value = value - 128;
        value = value * 256;
        var msb = (value >> 8);
        var lsb = (value & 0x00FF);
        buf[2 * i] = lsb;
        buf[2 * i + 1] = msb;
    }

    console.log('length:', databuf.length);
    // console.log(wavheader);
    headmod(buf.length + 36, 4, wavheader);
    headmod(bitrate, 24, wavheader);
    headmod(bitrate * bitspersample * channels / 8, 28, wavheader);
    headmod(buf.length, 40, wavheader);
    // console.log('---------------------------------------')
    // console.log(wavheader);
    var outfile = './public/wav/' + name + '.wav';
    fs.writeFileSync(outfile, Buffer.from(wavheader.map(c => typeof c === 'string' ? c.charCodeAt(0) : c)));
    fs.appendFileSync(outfile, buf);
    // var a = Buffer.from(wavheader);
    // console.log(a);
    console.log("done");
}

module.exports = createWAV;