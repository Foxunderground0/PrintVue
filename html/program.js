function beginLiveView() {
  switchPreviewType(true);
  document.getElementById("liveView").src ="./live-cam.jpg";
}

function switchPreviewType(showLive) {
  if (showLive) {
    document.getElementById("vPlayer").classList.add("hidden");
    document.getElementById("liveView").classList.remove("hidden");
    document.getElementById("liveView").src = "";
  } else {
    document.getElementById("vPlayer").classList.remove("hidden");
    document.getElementById("liveView").classList.add("hidden");
  }
}

//LoadVideo("gallery/s0/");
var videoList = [];
const downloadIcon = document.getElementById("download-icon");
function FetchAll() {
  fetch("gallery/sequences.json").then(async (resp) => {
    var thumbnails = (await resp.json()).list;

    const thumbnailRow = document.getElementById("thumbnailRow");
    thumbnailRow.innerHTML = ""; // Clear existing content

    var firstLoad = true;
    for (var seqI in thumbnails){
      sequence = thumbnails[seqI];
      
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
        currentVideo = vid;
        vid.Load();
      });
    }
  });
}
FetchAll();
