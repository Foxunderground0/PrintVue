// UI Logic
const imagePlayer = document.getElementById("imagePlayer");
const imageViewport = document.getElementById("imageViewport");
const playPauseButton = document.getElementById("playPauseButton");
const seekBar = document.getElementById("seekBar");
const seekBarFill = document.getElementById("seekBarFill");
const bufferBarFill = document.getElementById("bufferBarFill");
const spinner = document.getElementById("spinner");

function showSpinner() {
  spinner.style.display = "block";
}

function hideSpinner() {
  spinner.style.display = "none";
}

function togglePlayPause() {
  const playPauseButton = document.getElementById("playPauseButton");
  // Toggle the play/pause state
  // Toggle the play/pause symbol
  if (currentVideo.isPlaying) currentVideo.Pause();
  else currentVideo.Play();
  SetPlayIcon(currentVideo.isPlaying);
}
function showControls() {
  if (currentVideo) {
    if (playPauseButton.style.opacity != 0) return;
    playPauseButton.style.opacity = "80%";
    seekBar.style.opacity = "100%";
  }
  HideControlsOnTimeout();
}
var playPauseAutoHider = null;
function HideControlsOnTimeout() {
  // Auto hide play button now
  if (playPauseAutoHider) clearTimeout(playPauseAutoHider);
  playPauseAutoHider = setTimeout(() => {
    playPauseAutoHider = null;
    playPauseButton.style.opacity = "0%";
    seekBar.style.opacity = "0%";
  }, 3000);
}
function SetPlayIcon(isPlaying) {
  playPauseButton.innerHTML = isPlaying ? "&#10074;&#10074;" : "&#9658;";
  HideControlsOnTimeout();
}

function showImageFrame(imgData) {
  imageViewport.src = imgData;
}
function setSeekIndicator(fraction) {
  seekBarFill.style.width = `${fraction * 100}%`;
}
function setBufferIndicator(fraction) {
  bufferBarFill.style.width = `${fraction * 100}%`;
  bufferBarFill.style.display = fraction < 1 ? "block" : "none";
}
function seekFractionChanged(event) {
  const clickPosition = event.clientX - seekBar.getBoundingClientRect().left;
  const fraction = clickPosition / seekBar.clientWidth;
  console.log("Seek to:", fraction);
  currentVideo.Seek(fraction);
  setSeekIndicator(fraction);
  return fraction;
}
HideControlsOnTimeout();

seekBar.addEventListener("click", seekFractionChanged);
playPauseButton.addEventListener("click", togglePlayPause);
imagePlayer.addEventListener("mousemove", showControls);

// Data logic
var currentVideo = {};

function LoadVideo(root) {
  //console.log("Load Vid:", root);
  var vid = new Video(root);

  vid.OnFrameLoaded = (frame, progress) => {
    if (vid !== currentVideo) return;
    // show frame
    setSeekIndicator(progress);
    showImageFrame(frame.imageData);
    hideSpinner();
  };
  vid.OnVideoStopped = () => {
    if (vid !== currentVideo) return;
    togglePlayPause(currentVideo.isPlaying);
  };
  vid.OnVideoLoading = () => {
    if (vid !== currentVideo) return;
    showSpinner();
  };
  vid.OnVideoPlaying = () => {
    if (vid !== currentVideo) return;
    SetPlayIcon(vid.isPlaying);
  };
  vid.OnVideoPaused = () => {
    if (vid !== currentVideo) return;
    SetPlayIcon(vid.isPlaying);
  };
  vid.OnFrameBuffered = (progress) => {
    if (vid !== currentVideo) return;
    setBufferIndicator(progress);
  };

  if (!currentVideo) currentVideo = vid;
  return vid;
}
