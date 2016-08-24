crosFrameData = function(seq, startTime, frameElapsedTime, jsElapsedTime) {
  this.seq = seq;
  this.startTime = startTime;
  this.frameElapsedTime = frameElapsedTime;
  this.jsElapsedTime = jsElapsedTime;
}

crosFpsCounter = function() {
  this.totalElapsedTime = 0.0;
  this.totalRenderTime = 0.0;
  this.totalFrames = 0;
  this.buffer_size = 120;
  this.frameDataBuf = new Array();
}

crosFpsCounter.prototype.update = function(
        startTime, frameElapsedTime, jsElapsedTime) {
  this.totalFrameElapsedTime += frameElapsedTime;
  this.totalJSElapsedTime += jsElapsedTime;
  this.frameDataBuf[this.totalFrames % this.buffer_size] = new crosFrameData(
      this.totalFrames, startTime, frameElapsedTime, jsElapsedTime);
  this.totalFrames += 1;
}

crosFpsCounter.prototype.reset = function() {
  this.totalFrameElapsedTime = 0.0;
  this.totalJSElapsedTime = 0.0;
  this.totalFrames = 0;
  this.frameDataBuf = new Array();
}

crosFpsCounter.prototype.getAvgFps = function() {
  return this.totalFrames / this.totalFrameElapsedTime;
}

crosFpsCounter.prototype.getAvgRenderTime = function() {
  return this.totalJSElapsedTime / this.totalFrames;
}

crosFpsCounter.prototype.getFrameData = function() {
  return this.frameDataBuf;
}
