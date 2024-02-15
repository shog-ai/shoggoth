console.log("hello world")



// const api_url = '/api/';
const sync_rate = 1000;

// let peers_list;
// let pins_list;
// let manifest;

let first_run = true;

// function populate_peers_list(new_peers_list) {
//   if (first_run) {
//     // console.log("FIRST RUN!!");

//     peers_list = new_peers_list;
//   }

//   if (new_peers_list != peers_list || first_run) {
//     // console.log("PEERS CHANGE!!");

//     const container = document.getElementById("peers-items");

//     while (container.firstChild) {
//       container.removeChild(container.firstChild);
//     }

//     JSON.parse(new_peers_list).forEach(item => {
//       const div = document.createElement("div");
//       div.innerHTML = "<a href='" + item.host + "'>" + item.node_id + "</a>";
//       div.classList.add("peers-item");
//       container.appendChild(div);
//     });

//     // peers_list = new_peers_list;
//   }
// }

// function populate_pins_list(new_pins_list) {
//   if (first_run) {
//     // console.log("FIRST RUN!!");

//     pins_list = new_pins_list;
//   }

//   if (new_pins_list != pins_list || first_run) {
//     // console.log("PINS CHANGE!!");

//     const container = document.getElementById("pins-items");

//     while (container.firstChild) {
//       container.removeChild(container.firstChild);
//     }

//     JSON.parse(new_pins_list).forEach(item => {
//       const div = document.createElement("div");
//       div.innerHTML = "<a href='/explorer/resource/" + item.shoggoth_id + "'>" + item.shoggoth_id + "</a>";
//       div.classList.add("pins-item");
//       container.appendChild(div);
//     });

//     // pins_list = new_pins_list;
//   }
// }

// function populate_node_id(new_manifest) {
//   // if (first_run) {
//     // manifest = new_manifest;
//   // }

//   if (new_manifest != manifest || first_run) {
//     const container = document.getElementById("node_id");
//     container.textContent = JSON.parse(new_manifest).node_id;

//     // manifest = new_manifest;
//   }
// }

// function populate_node_host(new_manifest) {
//   // if (first_run) {
//     // manifest = new_manifest;
//   // }

//   if (new_manifest != manifest || first_run) {
//     const container = document.getElementById("node_host");
//     container.textContent = JSON.parse(new_manifest).public_host;

//     // manifest = new_manifest;
//   }
// }

// function populate_peers_count(new_peers_list) {
//   // if (first_run) {
//     // peers_list = new_peers_list;
//   // }

//   if (new_peers_list != peers_list || first_run) {
//     console.log("Here!!");
    
//     const container = document.getElementById("node_peers");
//     container.textContent = JSON.parse(new_peers_list).length;

//     // peers_list = new_peers_list;
//   }
// }

// function populate_pins_count(new_pins_list) {
//   // if (first_run) {
//     // pins_list = new_pins_list;
//   // }

//   if (new_pins_list != pins_list || first_run) {
//     const container = document.getElementById("node_pins");
//     container.textContent = JSON.parse(new_pins_list).length;

//     // pins_list = new_pins_list;
//   }
// }

async function set_models() {
//   // Make a GET request using the fetch API
//   await fetch(api_url + "get_manifest")
//     .then(response => {
//       // Check if the response status is OK (200)
//       if (!response.ok) {
//         throw new Error(`HTTP error! Status: ${response.status}`);
//       }
//       // Parse the response body as JSON
//       return response.text();
//     })
//     .then(data => {
//       // Handle the JSON data
//       // console.log(data);

//       populate_node_id(data);
//       populate_node_host(data);
    
//       manifest = data;
//     })
//     .catch(error => {
//       // Handle any errors that occurred during the fetch
//       console.error('Fetch error:', error);
//     });
}

async function sync_data() {
  console.log("Syncing data!");

  await set_models();
}

window.onload = async function() {
  await sync_data();

  first_run = false;

  setInterval(sync_data, sync_rate);
}

function send_message_pressed() {
  console.log("send pressed");
}

function mount_model_pressed() {
  console.log("mount pressed");
}

function add_chat_pressed() {
  console.log("mount pressed");
}
