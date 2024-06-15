window.onload = async function() {
  await main_load();
  
  document.getElementById("system-prompt").value = settings.chat_system_prompt;
  
  document.getElementById("shog-hub-url").value = settings.shog_hub_url;
  
  document.getElementById("eth-rpc").value = settings.eth_rpc;
  document.getElementById("base-rpc").value = settings.base_rpc;

  setInterval(ping_backend, ping_rate);
}

async function open_seed_phrase() {
  await fetch(api_url + "/open_seed_phrase")
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


async function save_pressed() {
  settings.chat_system_prompt = document.getElementById("system-prompt").value;
  
  settings.shog_hub_url = document.getElementById("shog-hub-url").value;
  
  settings.eth_rpc = document.getElementById("eth-rpc").value;
  settings.base_rpc = document.getElementById("base-rpc").value;

  
  await fetch(api_url + "/update_settings", {method: 'POST', body: JSON.stringify(settings)}
  )
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);
    
    toast("Saved", "success");
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });
}

async function force_exit() { 
  await fetch(api_url + "/force_exit")
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

    toast("Backend Exited", "warn");
  });
}

async function show_version() {
  console.log(version);

  toast_big("Version", version);
}
