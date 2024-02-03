class Video {
  constructor(root) {
    this.root = root;
    this.isLoaded = false;
    this.hasBuffered = false;
    this.isBuffering = false;
    this.OnFrameBuffered = null;
    this.OnFrameLoaded = null;
    this.currentIndex = 0;
    this.isPlaying = false;
    this.LoadingTimeout = null;
    this.OnVideoStopped = null;
    this.OnVideoLoading = null;
    this.period = 200;
    this.waitTimeOut = 3000;
    this.loop = false;
  }
  //Play Logic
  GetOrFetchFrame(index) {
    return new Promise((resolve, reject) => {
      if (this.frames[index].imageData) {
        console.log("Returning cached frame: ", index);
        resolve(this.frames[index]);
      } else {
        // all we can do is wait for now.
        console.log("waiting on:", index);
        if (this.LoadingTimeout) clearTimeout(this.LoadingTimeout);
        this.LoadingTimeout = setTimeout(() => {
          this.LoadingTimeout = null;
          if (this.OnVideoLoading)
            this.OnVideoLoading();
        }, this.waitTimeOut);
        this.frames[index].OnDataLoaded = () => {
          if (this.LoadingTimeout) clearTimeout(this.LoadingTimeout);
          this.LoadingTimeout = null;
          console.log("Wait ended on:", index);
          resolve(this.frames[index]);
        };
      }
    });
  }
  SendFrame(index) {
    console.log("SendFrame:", index);
    this.GetOrFetchFrame(index).then((frame) => {
      if (!isPlaying) {
        // playback was stopped
        return;
      }
      if (this.OnFrameLoaded) {
        this.currentIndex = index;
        this.OnFrameLoaded(frame, index / (this.frames.length - 1));
      }
      setTimeout(() => {
        if (index + 1 >= this.frames.length) {
          // video has ended
          if (this.loop) this.SendFrame(0);
          else {
            if (this.OnVideoStopped) this.OnVideoStopped();
            this.currentIndex = 0; // for the next time
            this.isPlaying = false;
          }
        } else this.SendFrame(index + 1);
      }, this.period);
    });
  }
  Pause() {
    if (!this.isPlaying) return;
    this.isPlaying = false;
  }
  Play() {
    console.log("Play while:", this.isPlaying);
    if (this.isPlaying) return;
    this.isPlaying = true;
    this.LoadMeta().then(() => {
      this.BeginBuffering();
      this.SendFrame(this.currentIndex);
    });
  }
  // Buffer Logic
  downloadFrame(index) {
    return new Promise((resolve, reject) => {
      setTimeout(() => {
        fetch(this.frames[index].file).then(async (resp) => {
          if (!resp.ok) {
            throw new Error("Network response was not ok");
          }
          const blob = await resp.blob();
          const objectURL = URL.createObjectURL(blob);

          // cache the data for future use
          this.frames[index].imageData = objectURL;
          console.log("Cached:", index);
          if (this.frames[index].OnDataLoaded)
            this.frames[index].OnDataLoaded();
          resolve();
        });
      }, 100); // simulate a delay
    });
  }
  bufferFrame(index) {
    if (!this.isBuffering)
        return;
    if (index >= this.frames.length) {
      this.hasBuffered = true;
      return;
    }
    if (this.frames[index].imageData) return;
    this.downloadFrame(index).then(() => {
      this.OnFrameBuffered(index / (this.frames.length - 1));
      this.bufferFrame(index + 1);
    });
  }
  BeginBuffering() {
    if(this.isBuffering){
        return;
    }
    this.isBuffering = true;
    this.LoadMeta().then(() => {
      this.bufferFrame(0);
    });
  }
  PauseBuffering() {
    this.isBuffering = false;
  }
  LoadMeta() {
    return new Promise((resolve, reject) => {
      if (this.isLoaded) {
        console.log("Already laoded");
        resolve();
        return;
      }
      fetch(this.root + "seq_info.json").then(async (res) => {
        if (!res.ok) reject();
        var meta_info = await res.json();
        this.meta = meta_info;
        console.log("Fetched meta:", meta_info);
        this.frames = [];
        for (
          var i = meta_info.startFrameIndex;
          i < meta_info.endFrameIndex;
          i++
        ) {
          this.frames.push({
            file: this.root + meta_info.seed + i + meta_info.extension,
            imageData: null,
            index: i,
            OnDataLoaded: null,
          });
        }
        console.log("Init Frames:", this.frames);
        this.isLoaded = true;
        resolve();
      });
    });
  }
}