NessStretchUGen : UGen {
	*ar { |bufIn, durMult=100, numSlices = 9, extreme = 0, paulStretchWinSize=1, mul=1, add=0|
		^this.multiNew('audio', bufIn, durMult, numSlices, extreme, paulStretchWinSize).madd(mul,add);
	}
	checkInputs {
		/* TODO */
		^this.checkValidInputs;
	}
}

PaulStretch {
	*ar {|buffer, durMult=20, startSample=0, extreme=0, paulStretchWinSize=1, mul=1, add=0|
		^NessStretch.ar(buffer, durMult, 1, startSample, extreme, paulStretchWinSize)
	}

	*renderNRT {|fileName, durMult = 100, startSample=0, extreme = 0, paulStretchWinSize = 1, dur = -1, outFileName|
		NessStretch.renderNRT(fileName, durMult, 1, 0, extreme, paulStretchWinSize, outFileName);
	}
}

NessStretch {
	*initClass {
		StartUp.add {
			SynthDef("NessStretchNRT", {arg buffer, durMult=100, numSlices=9, startSample=0, extreme=0, paulStretchWinSize=1, dur, outChan = 0;
				var env = EnvGen.ar(Env([0,1,1,0], [0,dur,0]), doneAction:2 );
				Out.ar(outChan, NessStretch.ar(buffer, durMult, numSlices, startSample, extreme, paulStretchWinSize)*env);
			}).writeDefFile;
		}
	}

	*ar{|buffer, durMult=100, numSlices = 9, startSample=0, extreme = 0, paulStretchWinSize=1|
		//var numSlices = (SampleRate.ir/44100).round+8; //double the buffer size if the SampleRate is above 48K

		var chunkSizeTimes2 = 65536*2*(SampleRate.ir/44100).round;
		var winSizeDivisor, localBuf, chain, demand, pos;

/*		if(numSlices == 9||numSlices == 10)
		{
			winSizeDivisor = 1
		}{
			winSizeDivisor = 2**(9-numSlices);
		};*/

		localBuf = LocalBuf(chunkSizeTimes2);

		chain = BufFFTTrigger(localBuf, 0.5/*/winSizeDivisor*/, 0, 1);

		demand = Dseries(startSample, chunkSizeTimes2/2/(durMult)*(BufRateScale.kr(buffer)));
		pos = Demand.kr(chain, 0, demandUGens: demand);

		chain = BufFFT_BufCopy(chain, buffer, pos, BufRateScale.kr(buffer), 0.5);
		^NessStretchUGen.ar(chain, durMult, numSlices, extreme, paulStretchWinSize);
	}

	*renderNRT {|fileName, durMult = 100, numSlices = 9, startSample=0, extreme = 0, paulStretchWinSize = 1, dur = -1, outFileName|

		var sf = SoundFile.openRead(fileName);

		var server = Server(("nrt"++NessStretch_Server_ID.next).asSymbol,
			options: Server.local.options
			.numOutputBusChannels_(sf.numChannels)
			.numInputBusChannels_(sf.numChannels)
		);

		var score = Score.new();

		var buffers = List.newClear(0);
		var time = Main.elapsedTime;

		"NessStretch!!!".postln;

		if (dur == -1) {
			dur = (sf.duration-(startSample/sf.sampleRate))*durMult+3
		} {
			dur = min((sf.duration-(startSample/sf.sampleRate))*durMult+3, dur)
		};

		if(sf!=nil){
			sf.numChannels.do{arg chanNum;
				buffers.add(Buffer.new(server, 0, 1));
				score.add([0.0, buffers[chanNum].allocReadChannelMsg(sf.path, 0, -1, [chanNum])]);
				score.add([0.0, Synth.basicNew((\NessStretchNRT), server).newMsg(args: [\buffer, buffers[chanNum].bufnum, \durMult, durMult, \numSlices, numSlices, \extreme, extreme, \paulStretchWinSize, paulStretchWinSize, \dur, dur, \outChan, chanNum])]);
			};

			outFileName = outFileName ?? (PathName(fileName).pathOnly++PathName(fileName).fileNameWithoutExtension++"_"++durMult++"."++PathName(fileName).extension);
			postf("writing file % \n \n", outFileName);

			score.recordNRT(
				outputFilePath: outFileName,
				sampleRate: sf.sampleRate,
				headerFormat: "w64",
				sampleFormat: "int32",
				options: server.options,
				duration: dur,
				action: {postf("done! % seconds \n", (Main.elapsedTime-time))}
			);
		} {
			"Sound File Empty!".postln;
		}


	}

	/*	*getServer{
	if(server==nil){
	serverNum = 57100+NRT_Server_Inc.next;
	while(
	{("lsof -i:"++serverNum++" ").unixCmdGetStdOut.size > 0},
	{serverNum = 57100+NRT_Server_Inc.next; serverNum.postln}
	);

	("server id: "++serverNum).postln;
	server = Server(("lang "++serverNum).asSymbol, NetAddr("127.0.0.1", serverNum),
	options: Server.local.options
	);
	};
	}*/
}

NessStretch_Server_ID {
	classvar <id=5000;
	*initClass { id = 5000; }
	*next  { ^id = id + 1; }
	*path {this.filenameSymbol.postln}
}