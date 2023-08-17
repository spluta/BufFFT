/*BufFFT : PV_ChainUGen {
	*new { | buffer, wintype = 0, winsize=0|
		^this.multiNew('control', buffer, wintype, winsize)
	}

	fftSize { ^BufFrames.ir(inputs[0]) }
}*/

BufFFT : PV_ChainUGen {
	*new { | buffer, wintype = 0, winsize=0|
		^this.multiNew('control', buffer, wintype, winsize)
	}

	fftSize { ^BufFrames.ir(inputs[0]) }
}

BufFFTTrigger : PV_ChainUGen {
	*new { | buffer, hop = 1, start = 0, step = 1, polar = 0.0|
		^this.multiNew('control', buffer, hop, start, step, polar)
	}
}

BufFFTTrigger2 : PV_ChainUGen {
	*new { | buffer, trigger|
		^this.multiNew('control', buffer, trigger)
	}
}

BufIFFT : WidthFirstUGen {
	*new { | buffer, wintype = 0, winsize=0|
		^this.ar(buffer, wintype, winsize)
	}

	*ar { | buffer, wintype = 0, winsize=0|
		^this.multiNew('audio', buffer, wintype, winsize)
	}

	*kr { | buffer, wintype = 0, winsize=0|
		^this.multiNew('control', buffer, wintype, winsize)
	}

}

BufFFT_BufCopy : PV_ChainUGen {
	*new { arg chainBuf, sourceBuf, startFrame, bufRateScale=1;
		//I am reversing the order here because the compiler resizes the chain buf if it is first
		^this.multiNew('control', chainBuf, sourceBuf, startFrame, bufRateScale)
	}
}

PV_AccumPhase : PV_MagMul {}