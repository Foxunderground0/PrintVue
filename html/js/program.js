function beginLiveView() {
  switchPreviewType(true);
}
function EndLiveView() {
  switchPreviewType(false);
}
var currentViewIsLive = false;
function switchPreviewType(showLive) {
  if (showLive) {
    document.getElementById("vPlayer").classList.add("hidden");
    document.getElementById("liveView").classList.remove("hidden");
    document.getElementById("liveView").src = "";
    currentViewIsLive = true;
    keepImageFresh();
  } else {
    document.getElementById("vPlayer").classList.remove("hidden");
    document.getElementById("liveView").classList.add("hidden");
    currentViewIsLive = false;
  }
}

//LoadVideo("gallery/s0/");
var videoList = [];
const downloadIcon = document.getElementById("download-icon");
function FetchAll() {
  fetch("gallery/sequences.json").then(async (resp) => {
    var thumbnails = (await resp.json()).list;

    const thumbnailRow = document.getElementById("thumbnailRow");

    var firstLoad = true;
    for (var seqI in thumbnails) {
      var sequence = thumbnails[seqI];
      //console.log("Adding:", sequence);

      const thumbnailCol = document.createElement("div");
      thumbnailCol.className = "col-xs-12 col-sm-6 col-md-6 col-lg-6"; // Adjust as needed
      thumbnailCol.classList.add("no-padding");
      const thumbnailContainer = document.createElement("div");
      thumbnailContainer.className = "thumbnail-container";
      thumbnailContainer.classList.add("thumbnail-container-loading");

      const thumbnailImg = document.createElement("img");
      //thumbnailImg.src = thumbnailSrc;
      thumbnailImg.className = "thumbnail";

      const spinner = document.createElement("div");
      spinner.id = "thumbnail-spinner";
      thumbnailContainer.appendChild(spinner);

      thumbnailContainer.appendChild(thumbnailImg);
      thumbnailCol.appendChild(thumbnailContainer);
      thumbnailRow.appendChild(thumbnailCol);

      var vid = LoadVideo("gallery/" + sequence + "/");
      vid.thumbnailImg = thumbnailImg;
      vid.thumbnailContainer = thumbnailContainer;
      vid.spinner = spinner;
      var img = await vid.LoadThumbnail();
      thumbnailImg.src = img;

      if (firstLoad) {
        firstLoad = false;
        console.log("setting default");
        currentVideo = vid;
      }
      vid.thumbnailContainer.classList.remove("thumbnail-container-loading");
      vid.spinner.style.display = "none";
      videoList.push(vid);

      // Event handlers
      thumbnailImg.addEventListener("click", () => {
        for (v in videoList) {
          videoList[v].thumbnailImg.classList.remove("selected-thumbnail");
        }
        thumbnailImg.classList.add("selected-thumbnail");
        EndLiveView();
        currentVideo = vid;
        vid.Load();
      });
    }
    var msg = document.getElementById("thumbnail-loading-msg");
    console.log("msg", msg);
    msg.innerHTML =
      thumbnails.length > 0 ? "" : "No time lapses have been recorded yet.";
    console.log("Thumnails Length: ", thumbnails.length);
  });
}

setTimeout(() => {
  FetchAll();
}, 0);

var inCamFetch = false;
function keepImageFresh() {
  if (inCamFetch) return;
  inCamFetch = true;
  fetchCamImage().then((img) => {
    inCamFetch = false;
    console.log("Setting image: ", img);
    document.getElementById("liveView").src = img;
    if (currentViewIsLive) keepImageFresh();
  });
}
function fetchCamImage() {
  return new Promise((resolve, reject) => {
    console.log("Fetch cam image");
    fetch("./refresh-cam", { method: "POST" })
      .then(async (resp) => {
        console.log("Cam image fresh");
        // fetch image now
        await fetch("./live-cam.jpg")
          .then(async (resp) => {
            console.log("Got cam Image: ", resp);
            const blob = await resp.blob();
            const objectURL = URL.createObjectURL(blob);
            resolve(objectURL);
          })
          .catch((reason) => {
            console.log("Couldn't fetch image");
            //reject(reason);
          });
      })
      .catch((reason) => {
        console.log("Post failed");
        //reject(reason);
      });
  });
}
