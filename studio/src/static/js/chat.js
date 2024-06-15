const sync_rate = 500;

let old_state;
let new_state;

let old_active_chat = 0;
let active_chat = 0;

let first_run = true;
let waiting = false;
let accumulated = "";

let model_server_url = "http://127.0.0.1:8000";

async function update_models_ui() { 
  if(!arraysAreEqual(old_state.models, new_state.models) || first_run) {
    console.log("models changed");
    
    const models_select = document.getElementById("model-select");

    models_select.innerHTML = "";

    new_state.models.forEach(function(model) {
      const newOption = document.createElement("option");
      newOption.text = model.name;
      newOption.value = model.name.toLowerCase().replace(/\s+/g, '-');
      models_select.appendChild(newOption);
    });

    if (new_state.models.length == 0) {
      models_select.innerHTML = '<option value="" disabled selected>select model</option>';
    }
  }

  if(!deepEqual(old_state.state, new_state.state) || first_run) {
    console.log("chat state changed");
    
    if(new_state.state.mount_status == "Unmounted") {
      document.getElementById('mount-btn').style.display = 'flex';
      document.getElementById('unmount-btn').style.display = 'none';
      document.getElementById('mounting-btn').style.display = 'none';
      
      document.getElementById('model-select').disabled = false;
    } else if(new_state.state.mount_status == "Mounting") {
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

}

async function update_sessions_ui() {   
  if(!deepEqual(old_state.state, new_state.state) || first_run || old_active_chat != active_chat) {
    const chat_items = document.querySelector(".chat-items");

    chat_items.innerHTML = "";

    for (let i = 0; i < new_state.state.sessions.length; i++) {
      let chat = new_state.state.sessions[i];
    
      const div = document.createElement("div");
      div.classList.add("chat-item");

      div.dataset.index = i;

      div.onclick = function() {
          chat_item_pressed(div);
      }

      if (i == active_chat) {
        div.classList.add("chat-item-active");
      }
    
      div.innerHTML = `
        <div>` + chat.name +`</div>
        <img data-index="` + i + `" src="/static/img/bin.svg" class="chat-options" title="delete" onclick="chat_options_pressed(this, event)" />
      `;
      
      chat_items.appendChild(div);
    }
  }
}

let new_chat_str = `
  <div class="new-chat-view">
    <img class="new-chat-view-img" src="/static/img/icon.png"/>
    <div class="new-chat-view-txt">How can I help you today?</div>
  </div>
`;

async function update_chat_view() {
  if (new_state.state.sessions.length == 0) {
    document.getElementById('chat-view').innerHTML = new_chat_str;
    
    return;
  }
 
  if(!deepEqual(old_state.state, new_state.state) || first_run || old_active_chat != active_chat) {
    if(!waiting) {
  
      document.querySelector(".chat-view").innerHTML = "";
  
      let history = new_state.state.sessions[active_chat].messages;

      history.forEach((item) => {
        if (item.role == "user") {
          view_add_msg("User", item.content);     
        } else if (item.role == "assistant") {      
          view_add_msg("AI", item.content);
        }
      });

      if (history.length < 2) {
        document.getElementById('chat-view').innerHTML = new_chat_str;
      }
      
      hljs.highlightAll();
    }
  }
}

async function update_ui() { 
  if(first_run) {
    old_state = new_state;
    old_active_chat = active_chat;
  }

  await update_models_ui();
  
  await update_sessions_ui();
  
  await update_chat_view();

  old_state = new_state;
  old_active_chat = active_chat;
}

async function sync_state() {
  // console.log("Syncing state!");

  await fetch(api_url + "/get_chat_state")
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    return response.text();
  })
  .then(data => {
    // console.log(data);
    
    new_state = JSON.parse(data);
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });

  await update_ui();
}


function view_add_msg(from, msg_str) {  
  let new_div = document.createElement("div");
  new_div.classList.add("chat-view-item");

  if (from == "AI" && waiting) {
    new_div.classList.add("pending-response");
  }

  let img;

  if (from == "AI") {
    img = "icon.png";
  } else {
    img = "account.svg";
  }

  let converter = new showdown.Converter();
  let html = converter.makeHtml(msg_str);

  msg_str = html;
  
  new_div.innerHTML =
  ` 
  <img src=\"/static/img/` + img + `\" class=\"chat-view-item-img\" />
  
  <div class=\"chat-view-item-right\">
    <div class=\"chat-view-item-right-name\"><b>` + from + `</b></div>
    <div class=\"chat-view-item-right-msg\">` + msg_str + `</div>
  </div>
  `

  if (from == "AI" && waiting) {
    new_div.innerHTML =
    ` 
    <img src=\"/static/img/` + img + `\" class=\"chat-view-item-img\" />
  
    <div class=\"chat-view-item-right\">
      <div class=\"chat-view-item-right-name\"><b>` + from + `</b></div>
      <div class=\"chat-view-item-right-msg pending-response-msg\">` + msg_str + `</div>
    </div>
    `
  }
  
  let chat_view = document.getElementById('chat-view');
  chat_view.appendChild(new_div);
}

function view_add_network_error() {
  let new_div = document.createElement("div");
  new_div.classList.add("chat-view-item");
  new_div.classList.add("chat-view-item-network-error");

  new_div.innerHTML = "Network Error, model server unreachable (try remounting model)";
  
  let chat_view = document.getElementById('chat-view');
  chat_view.appendChild(new_div);
}

async function add_message(session_index, role, content) {
  await fetch(api_url + "/chat_add_message", {
   method: 'POST',
   headers: {
     'Content-Type': 'application/json',
   },
   body: JSON.stringify({
      session_index,
      role,
      content,
   }),
   })
   .then(response => {
     if (!response.ok) {
       throw new Error(`HTTP error! Status: ${response.status}`);
     }
     return response.text();
   })
   .then(data => {
   })
   .catch(error => {
     console.error('Fetch error:', error);
   });
}

async function send_message_pressed() {
  console.log("send pressed");

  if(new_state.state.mount_status != "Mounted") {  
    toast("model not mounted","warn");
  
    return;
  }   
  
  if (waiting) {
    return;
  }

  if (new_state.state.sessions.length == 0) {
    await add_session("new chat");
  }

  let msg_str = document.getElementById('message-input').value;

  document.getElementById('message-input').value = "";
  
  await sleep(1000);
  
  await add_message(active_chat, "user", msg_str);
 
  document.querySelector('.send-btn').classList.add("send-btn-disabled");

  await update_ui();

  await sleep(1000);
  
  waiting = true;

  view_add_msg("AI", "");

  let failed = false;

  try {
    let messages = new_state.state.sessions[active_chat].messages;
    console.log(messages);
    
    const response =  await fetch(model_server_url + "/v1/chat/completions", {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        messages: messages,
        stream: true,
        max_tokens: 20000,
        temperature: 0.7,
      }),
    });
  
    if (!response.ok) {
      throw new Error(`HTTP error! status: ${response.status}`);
    }

    const reader = response.body.getReader();
    const decoder = new TextDecoder('utf-8');
    let done = false;

    while (!done) {
      const { value, done: readerDone } = await reader.read();
      done = readerDone;
      if (value) {
        const chunk = decoder.decode(value, { stream: true });
        const lines = chunk.split('\n').filter(Boolean);

        for (const line of lines) {
          let line_stripped = line.replace('data: ', '');
          // console.log("LINE: " + line_stripped);

          if (line_stripped[0] == '{') {
            let line_json = JSON.parse(line_stripped);
            // console.log(line_json);

            if (line_json.choices[0].finish_reason == null && line_json.choices[0].delta.content != undefined) {
              let new_data = line_json.choices[0].delta.content;
              console.log(new_data);

              accumulated = accumulated + new_data;
              
              let converter = new showdown.Converter();
              let html = converter.makeHtml(accumulated);

              document.querySelector(".pending-response-msg").innerHTML = html;

              hljs.highlightAll();
            }
          } else if (line_stripped == "[DONE]") {
            break;
          }          
        }
      }
    }
  } catch (error) {
    view_add_network_error();

    toast("Network Error", "warn");
    
    console.error('Fetch error:', error);
    
    failed = true;
  }

  if (failed) {
    accumulated = "";
    waiting = false;

    document.querySelector('.send-btn').classList.remove("send-btn-disabled");
    
    await update_ui();

    return;
  } else {
    await add_message(active_chat, "assistant", accumulated);
    
    accumulated = "";
    waiting = false;
  
    document.querySelector(".pending-response").classList.remove("pending-response");
    document.querySelector(".pending-response-msg").classList.remove("pending-response-msg");

    await sleep(1000);

    document.querySelector('.send-btn').classList.remove("send-btn-disabled");
    await update_ui();
   }
}

async function chat_item_pressed(element) {
  let index = element.dataset.index;
  
  console.log("chat item pressed: " + index);

  active_chat = parseInt(index);

  await sync_state();
  await update_ui();
}

async function chat_options_pressed(element, event) {
  event.stopPropagation();

  let index = element.dataset.index;
  
  console.log("chat options pressed: " + index);

  index = parseInt(index);

  await fetch(api_url + "/chat_delete_session/" + index)
   .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    return response.text();
  })
    .then(data => {
  })
  .catch(error => {
     console.error('Fetch error:', error);
  });
  
  active_chat = 0;

  await sync_state();
  await update_ui();
}

async function add_session(name) {
  await fetch(api_url + "/chat_add_session", {
     method: 'POST',
     headers: {
       'Content-Type': 'application/json',
     },
     body: JSON.stringify({name}),
   })
   .then(response => {
     if (!response.ok) {
       throw new Error(`HTTP error! Status: ${response.status}`);
     }
     return response.text();
   })
   .then(data => {
   })
   .catch(error => {
     console.error('Fetch error:', error);
   });
}

async function add_chat_pressed() {
  console.log("add chat pressed");


  add_session("new chat").then(async function() {
    await sync_state();
    
    active_chat = new_state.state.sessions.length - 1;

    await update_ui();
  });
}

async function mount_model_pressed() {
  console.log("mount pressed");

  let select_element = document.getElementById('model-select');

  let model_name = select_element.value;
  
  await fetch(api_url + "/mount_model/" + model_name)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    return response.text();
  })
  .then(async function(data) {
    await update_ui();
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });
}

async function unmount_model_pressed() {
  console.log("unmount pressed");

  await fetch(api_url + "/unmount_model")
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    return response.text();
  })
  .then(async function(data) {  
    await update_ui();
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });
}

async function handle_message_key_press(event) {
  if ((event.key === 'Enter' || event.keyCode === 13) && !event.shiftKey) {
    await send_message_pressed();
  }
}

window.onload = async function() {
  await main_load();
  
  await sync_state();

  first_run = false;

  setInterval(sync_state, sync_rate);

  setInterval(ping_backend, ping_rate);
  
  document.getElementById('message-input').addEventListener("input", function(){
    this.style.height = '0px';
    this.style.height = this.scrollHeight + 'px';
  });
}
