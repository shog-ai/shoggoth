console.log("hello world")



const api_url = '/api/';
const sync_rate = 1000;

let studio_state;
let state_models;


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

async function set_models() {
  if(first_run) {
    state_models = studio_state.models;
  }

  if(!arraysAreEqual(state_models, studio_state.models) || first_run) {
    console.log("models changed");
    
    const models_select = document.getElementById("model-select");

    models_select.innerHTML = "";

    studio_state.models.forEach(function(optionText) {
      // Create a new <option> element
      const newOption = document.createElement("option");
      // Set the text and value for the option
      newOption.text = optionText.name;
      newOption.value = optionText.name.toLowerCase().replace(/\s+/g, '-');
      // Append the option to the <select> element
      models_select.appendChild(newOption);
    });
  }

  state_models = studio_state.models;
}

async function update_ui() {
  // console.log(studio_state);  
  
  await set_models();
}

async function sync_state() {
  // console.log("Syncing state!");

  // Make a GET request using the fetch API
  await fetch(api_url + "get_state")
  .then(response => {
    // Check if the response status is OK (200)
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    // Parse the response body as JSON
    return response.text();
  })
  .then(data => {
    // Handle the JSON data
    // console.log(data);
    
    studio_state = JSON.parse(data);

  })
  .catch(error => {
    // Handle any errors that occurred during the fetch
    console.error('Fetch error:', error);
  });

  await update_ui();
}

window.onload = async function() {
  await sync_state();

  first_run = false;

  setInterval(sync_state, sync_rate);
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


function deepEqual(obj1, obj2) {
  if (obj1 === obj2) {
    return true;
  }

  if (typeof obj1 !== 'object' || typeof obj2 !== 'object' || obj1 == null || obj2 == null) {
    return false;
  }

  const keys1 = Object.keys(obj1);
  const keys2 = Object.keys(obj2);

  if (keys1.length !== keys2.length) {
    return false;
  }

  for (let key of keys1) {
    if (!keys2.includes(key) || !deepEqual(obj1[key], obj2[key])) {
      return false;
    }
  }

  return true;
}

function arraysAreEqual(arr1, arr2) {
  if (arr1.length !== arr2.length) {
    return false;
  }

  for (let i = 0; i < arr1.length; i++) {
    if (!deepEqual(arr1[i], arr2[i])) {
      return false;
    }
  }

  return true;
}
