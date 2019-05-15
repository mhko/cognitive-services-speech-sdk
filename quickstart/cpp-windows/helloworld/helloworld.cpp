//
// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.md file in the project root for full license information.
//

#include "stdafx.h"

#include <string.h>
#include <iostream>
#include <chrono>
#include <map>
#include <thread>
#include <speechapi_cxx.h>

#include <speechapi_cxx_speech_bot_connector.h>
#include <speechapi_cxx_audio_config.h>

using namespace Microsoft::CognitiveServices::Speech;

/* 
1. Update path to the input wave file. 
2. Update credentials in createBotConnectorConfig(). 
*/

static std::string waveFileName = "D:\\perf\\audio\\ShutDownTheComputer.wav";

std::shared_ptr<Dialog::BotConnectorConfig> createBotConnectorConfig() {
	auto config = Dialog::BotConnectorConfig::FromSecretKey("<bot id>", "<subscription key>", "<region>");
	config->SetSpeechRecognitionLanguage("en-us");
	return config;
}

void doOneTurn() {
	auto botConnectorConfig = createBotConnectorConfig();
	auto audioConfig = Audio::AudioConfig::FromWavFileInput(waveFileName);
	auto connector = Dialog::SpeechBotConnector::FromConfig(botConnectorConfig, audioConfig);

	if (connector == nullptr)
	{
		std::cout << "Unable to create a speech bot connector." << std::endl;
		return;
	}
	std::mutex m;
	std::condition_variable cv;

	connector->ActivityReceived += [&](const Dialog::ActivityReceivedEventArgs& event)
	{
		auto activity = event.GetActivity();
		auto text = activity->Serialize();
		std::cout << "### Activity Received: " << text << std::endl;

		uint8_t* buffer = new uint8_t[5000000];

		bool hasAudio = event.HasAudio();
		if (hasAudio) {
			std::cout<<"waiting for audio...\n";
			std::shared_ptr<Audio::PullAudioOutputStream> audio = event.GetAudio();
			std::cout<<"receiving audio.";
			while (audio->Read(buffer, 4096) != 0) { // filled size != 0
				std::cout<<".";
			}
			std::cout<<"!\n";
			std::cout<<"audio received!\n";
			cv.notify_one();
		}
		else
		{
			std::cout<<"no audio to wait for!\n";
		}
	};

	connector->Recognized += [&](const SpeechRecognitionEventArgs& event)
	{
		auto result = event.Result;
		auto text = result->Text;
		auto id = event.SessionId;
		std::cout << "### Recognized: " << id << " " << text << std::endl;
	};

	connector->SessionStarted += [&](const SessionEventArgs&)
	{
		std::cout << "### SessionStarted: " << std::endl;
	};

	connector->SessionStopped += [&](const SessionEventArgs&)
	{
		std::cout << "### SessionStopped: " << std::endl;
	};

	connector->Canceled += [&](const SpeechRecognitionCanceledEventArgs& event)
	{
		std::cout << "### Cancelled: " << std::endl;
		std::cout << "result : " << event.Result << std::endl;
		std::cout << "code : " << (int)event.ErrorCode << std::endl;
		std::cout << "details : " << event.ErrorDetails << std::endl;
	};

	connector->ConnectAsync().wait();
	connector->ListenOnceAsync().get();

	std::unique_lock<std::mutex> lk(m);
	cv.wait_for(lk, std::chrono::seconds(5));

	connector->DisconnectAsync().wait();

	std::cout << "waiting 2 secs.." << "\n";
	std::this_thread::sleep_for(std::chrono::seconds(2));
}

// not used 
/*
void recognizeSpeech()
{
    // Creates an instance of a speech config with specified subscription key and service region.
    // Replace with your own subscription key and service region (e.g., "westus").
    auto config = SpeechConfig::FromSubscription("YourSubscriptionKey", "YourServiceRegion");

    // Creates a speech recognizer.
    auto recognizer = SpeechRecognizer::FromConfig(config);
    std::cout << "Say something...\n";

    // Starts speech recognition, and returns after a single utterance is recognized. The end of a
    // single utterance is determined by listening for silence at the end or until a maximum of 15
    // seconds of audio is processed.  The task returns the recognition text as result. 
    // Note: Since RecognizeOnceAsync() returns only a single utterance, it is suitable only for single
    // shot recognition like command or query. 
    // For long-running multi-utterance recognition, use StartContinuousRecognitionAsync() instead.
    auto result = recognizer->RecognizeOnceAsync().get();

    // Checks result.
    if (result->Reason == ResultReason::RecognizedSpeech)
    {
		std::cout << "We recognized: " << result->Text << std::endl;
    }
    else if (result->Reason == ResultReason::NoMatch)
    {
		std::cout << "NOMATCH: Speech could not be recognized." << std::endl;
    }
    else if (result->Reason == ResultReason::Canceled)
    {
        auto cancellation = CancellationDetails::FromResult(result);
		std::cout << "CANCELED: Reason=" << (int)cancellation->Reason << std::endl;

        if (cancellation->Reason == CancellationReason::Error) 
        {
			std::cout << "CANCELED: ErrorCode= " << (int)cancellation->ErrorCode << std::endl;
			std::cout << "CANCELED: ErrorDetails=" << cancellation->ErrorDetails << std::endl;
            std::cout << "CANCELED: Did you update the subscription info?" << std::endl;
        }
    }
}
*/

int wmain()
{
	doOneTurn();
    return 0;
}
// </code>
