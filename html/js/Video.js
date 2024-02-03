class Video {
  constructor(root) {
    this.root = root;
    this.isLoaded = false;
    this.hasBuffered = false;
    this.OnFrameBuffered = null;
    this.OnFrameLoaded = null;
    this.currentIndex = 0;
    this.isPlaying = 0;
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
        this.frames[index].OnDataLoaded = () => {
          console.log("Wait ended on:", index);
          resolve(this.frames[index]);
        };
      }
    });
  }
  SendFrame(index) {
    this.GetOrFetchFrame(index).then(frame => {
      if (this.OnFrameLoaded)
        this.OnFrameLoaded(frame, index / (this.frames.length - 1));
      this.SendFrame(index + 1);
    });
  }
  Pause() {
    i;
  }
  Play() {
    this.LoadMeta().then(() => {
      this.BeginBuffering();
      this.SendFrame(this.currentIndex);
    });
  }
  // Buffer Logic
  downloadFrame(index) {
    return new Promise((resolve, reject) => {
      if (index >= this.frames.length) {
        this.hasBuffered = true;
        console.log("All Loaded");
        return;
      }
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
    this.downloadFrame(index).then(() => {
      this.OnFrameBuffered(index / (this.frames.length - 1));
      this.bufferFrame(index + 1);
    });
  }
  BeginBuffering() {
    console.log("Start Video: ", this);
    if (!this.isLoaded) {
      this.LoadMeta().then(() => {
        this.bufferFrame(0);
      });
    } else this.bufferFrame(0);
  }
  LoadMeta() {
    return new Promise((resolve, reject) => {
      if (this.isLoaded) resolve();
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
