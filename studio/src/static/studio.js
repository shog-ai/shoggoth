console.log("hello anon!")

const api_url = '/api/';
const sync_rate = 500;

let studio_state;
let state_models;
let state_active_model_status;

let first_run = true;

let chat_history = "This is a conversation between User and Llama, a friendly chatbot. Llama is helpful, kind, honest, good at writing, and never fails to answer any requests immediately and with precision.\n\nUser: ";


async function set_models() {
  if(first_run) {
    state_models = studio_state.models;
    state_active_model_status = studio_state.active_model.status;
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

  if(state_active_model_status != studio_state.active_model.status || first_run) {
    if(studio_state.active_model.status == "unmounted") {
      document.getElementById('mount-btn').style.display = 'flex';
      document.getElementById('unmount-btn').style.display = 'none';
      document.getElementById('mounting-btn').style.display = 'none';
      
      document.getElementById('model-select').disabled = false;
    } else if(studio_state.active_model.status == "mounting") {
      document.getElementById('mount-btn').style.display = 'none';
      document.getElementById('unmount-btn').style.display = 'none';
      document.getElementById('mounting-btn').style.display = 'flex';

      document.getElementById('model-select').disabled = true;
    } else {
      document.getElementById('mount-btn').style.display = 'none';
      document.getElementById('unmount-btn').style.display = 'flex';
      document.getElementById('mounting-btn').style.display = 'none';
    
      document.getElementById('model-select').disabled = true;
    }
  }

  state_models = studio_state.models;
  state_active_model_status = studio_state.active_model.status;
}

async function update_ui() {
  // console.log(studio_state);  
  
  await set_models();
}

async function sync_state() {
  // console.log("Syncing state!");

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

function add_view_msg(from, msg_str) {  
  let new_div = document.createElement("div");
  new_div.classList.add("chat-view-item");
  
  new_div.innerHTML =
  ` 
  <img src=\"/static/img/icon.png\" class=\"chat-view-item-img\" />
  
  <div class=\"chat-view-item-right\">
    <div class=\"chat-view-item-right-name\"><b>` + from + `</b></div>
    <div class=\"chat-view-item-right-msg\">` + msg_str + `</div>
  </div>
  `
  let chat_view = document.getElementById('chat-view');
  chat_view.appendChild(new_div);
}

async function send_message_pressed() {
  console.log("send pressed");

  let msg_str = document.getElementById('search-input').value;
  add_view_msg("You", msg_str);

  add_view_msg("AI", "...");

  
  await fetch(api_url + "completion", {
  method: 'POST',
  headers: {
    'Content-Type': 'application/json',
  },
  body: JSON.stringify({
    prompt: chat_history + msg_str + "\nLlama:",
  }),
  })
  .then(response => {
    // Check if the response status is OK (200)
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    return response.text();
  })
  .then(data => {
    // Handle the JSON data
    let resp = JSON.parse(data);

    add_view_msg("AI", resp.content);
  })
  .catch(error => {
    // Handle any errors that occurred during the fetch
    console.error('Fetch error:', error);
  });
}

async function mount_model_pressed() {
  console.log("mount pressed");

  let select_element = document.getElementById('model-select');

  let model_name = select_element.value;
  
  await fetch(api_url + "mount_model/" + model_name)
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
    console.log(data);
    
  })
  .catch(error => {
    // Handle any errors that occurred during the fetch
    console.error('Fetch error:', error);
  });
}

async function unmount_model_pressed() {
  console.log("unmount pressed");

  
  await fetch(api_url + "unmount_model")
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
    console.log(data);
    
  })
  .catch(error => {
    // Handle any errors that occurred during the fetch
    console.error('Fetch error:', error);
  });
}

async function add_chat_pressed() {
  console.log("add chat pressed");
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
