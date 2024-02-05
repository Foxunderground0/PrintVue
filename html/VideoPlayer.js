// UI Logic
const bufferBarFill = document.getElementById("bufferBarFill");
const spinner = document.getElementById("spinner");

function showSpinner() {
  spinner.style.display = "block";
}

function hideSpinner() {
  spinner.style.display = "none";
}

function setBufferIndicator(fraction) {
  bufferBarFill.style.width = `${fraction * 100}%`;
  bufferBarFill.style.display = fraction < 1 ? "block" : "none";
  if (fraction == 1){
  }
}
function setEncodeIndicator(fraction) {
  bufferBarFill.style.width = `${fraction * 100}%`;
  bufferBarFill.style.display = fraction < 1 ? "block" : "none";
  if (fraction == 1){
  }
}

// Data logic
var currentVideo = {};

function LoadVideo(root) {
  //console.log("Load Vid:", root);
  var vid = new Video(root);

  vid.OnVideoEncoded = (src) =>{
    document.getElementById("vPlayer").src = src;
  }
  vid.OnFrameBuffered = (progress) => {
    if (vid !== currentVideo) return;
    setBufferIndicator(progress);
  };

  vid.OnFrameEncoded = (progress) => {
    if (vid !== currentVideo) return;
    setEncodeIndicator(progress);
  };

  if (!currentVideo) currentVideo = vid;
  return vid;
}
