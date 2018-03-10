'use strict';
process.env.GOOGLE_APPLICATION_CREDENTIALS = "./voicenotewallet.json"
    // [START speech_quickstart]
    // Imports the Google Cloud client library
const speech = require('@google-cloud/speech');
const fs = require('fs');

// The name of the audio file to transcribe
// const fileName = './public/wav/2018_3_4_11_39_45.wav';

// Your Google Cloud Platform project ID
const projectId = 'voicenotewallet';


// returns a promise whose result is the
// transcribed text
function getSpeechToText(filename) {

    // Creates a client
    const client = new speech.SpeechClient({
        projectId: projectId,
    });

    // Reads a local audio file and converts it to base64
    const file = fs.readFileSync('./public/wav/' + filename + '.wav');
    const audioBytes = file.toString('base64');

    // The audio file's encoding, sample rate in hertz, and BCP-47 language code
    const audio = {
        content: audioBytes,
    };
    const config = {
        encoding: 'LINEAR16',
        sampleRateHertz: 10000,
        languageCode: 'en-US',
    };
    const request = {
        audio: audio,
        config: config,
    };

    // Detects speech in the audio file
    return client
        .recognize(request)
        .then(data => {
            const response = data[0];
            const transcription = response.results
                .map(result => result.alternatives[0].transcript)
                .join('\n');
            fs.writeFileSync('./public/wav/' + filename + '.txt', transcription)
            console.log(`Transcription: ${transcription}`);
            return transcription;
        })
        .catch(err => {
            console.error('ERROR:', err);
        });
}

module.exports = getSpeechToText;