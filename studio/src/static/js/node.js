async function delete_pressed(shoggoth_id) {
  console.log("delete pressed " + shoggoth_id);
  
  await fetch(api_url + "/node_delete_resource/" + shoggoth_id)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {       
    toast("Deleted", "remove");
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });

}

async function pause_pressed(shoggoth_id) {
  console.log("pause pressed " + shoggoth_id);
  
  await fetch(api_url + "/node_pause_resource/" + shoggoth_id)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {        
    toast("Paused", "pause");
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });

}

async function unpause_pressed(shoggoth_id) {
  console.log("unpause pressed " + shoggoth_id);
  
  await fetch(api_url + "/node_unpause_resource/" + shoggoth_id)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {        
    toast("Resumed", "success");
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });

}

async function open_downloads_folder() {
  await fetch(api_url + "/open_downloads_folder")
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


let template_str = `
  {{#each this}}
          <div class="main-item main-item-{{this.category}}">
            <a href="/hub/resource/{{this.namespace}}/{{this.name}}" class="main-item-top">
              <div class="main-item-user">
                <img class="user-img" src="{{../hub_url}}/static/img/users/{{this.namespace}}.png"/>
                <div class="main-item-title">{{this.namespace}} / {{this.name}}</div>
              </div>
        
              <div class="main-item-top-last">
                <div class="main-item-type">
                  <img class="main-item-model-icon" src="/static/img/{{this.category}}.svg"/>
                  <div class="main-item-type-txt">{{this.category}}</div>
                </div>
              </div>
            </a>
        
            <div class="main-item-bottom">
              <div>Status: {{status}}</div>
            </div>
            
            <div class="main-item-bottom">
              <div>progress: {{this.download_progress}}</div>
            </div>
             
            <div class="main-item-bottom">
              {{#if this.is_downloading}}<div class="control-btn control-btn-pause" onclick="pause_pressed('{{this.shoggoth_id}}')">Pause</div>{{/if}}
              {{#if this.is_paused}}<div class="control-btn control-btn-resume" onclick="unpause_pressed('{{this.shoggoth_id}}')">Resume</div>{{/if}}
              <div class="control-btn control-btn-delete" onclick="delete_pressed('{{this.shoggoth_id}}')">Delete</div>
            </div>
          </div>
    
        {{else}}
          <div>no resources</div>  
        {{/each}}
`;

let resources;
let node_info;
let node_stats;

function set_resources() {
  // console.log(resources);

  resources.forEach((item) => {
    item.category = item.category.toLowerCase();

    if (item.status == "Downloading") {
      item.is_downloading = true;
    } else if (item.status == "Paused") {
      item.is_paused = true;
    }
  });
  
  resources.hub_url = hub_url;
    
  const template = Handlebars.compile(template_str);
  let compiled = template(resources);
    
  // console.log(compiled);
  
  const parent = document.querySelector(".node-items");
  parent.innerHTML = compiled;
}

function set_info() {
  document.getElementById("node-id").innerHTML = node_info.node_id;

  if (node_info.status == "Online") {
    document.querySelector(".node-online").style.display = "flex";
    document.querySelector(".node-offline").style.display = "none";
  } else {
    document.querySelector(".node-online").style.display = "none";
    document.querySelector(".node-offline").style.display = "flex";
  }
}

let first_time = true;
let total_nodes_chart;
let online_nodes_chart;
let storage_used_chart;
let download_bandwidth_chart;
let upload_bandwidth_chart;

function format_time(timestamp) {
  const date = new Date(timestamp);
  let hours = date.getHours();
  let minutes = date.getMinutes();
  hours = hours < 10 ? '0' + hours : hours;
  minutes = minutes < 10 ? '0' + minutes : minutes;

  return hours + ':' + minutes;
}

function set_stats() {
  // console.log(node_stats);

  let to_retain = 1;

  let total_nodes_labels = [];
  let total_nodes_data = [];
  for (let i = 0; i < node_stats.total_nodes.length; i += to_retain) {
    let item = node_stats.total_nodes[i];
  
    total_nodes_labels.push(format_time(item[0]));
    
    total_nodes_data.push(item[1]);
  }
   
  let online_nodes_labels = [];
  let online_nodes_data = [];
  for (let i = 0; i < node_stats.online_nodes.length; i += to_retain) {
    let item = node_stats.online_nodes[i];
    
    online_nodes_labels.push(format_time(item[0]));
    
    online_nodes_data.push(item[1]);
  }
  
  let storage_used_labels = [];
  let storage_used_data = [];
  for (let i = 0; i < node_stats.storage_used.length; i += to_retain) {
    let item = node_stats.storage_used[i];
  
    storage_used_labels.push(format_time(item[0]));
    
    storage_used_data.push(format_bytes_gb(item[1]));
  }
    
  let download_bandwidth_labels = [];
  let download_bandwidth_data = [];
  for (let i = 0; i < node_stats.download_bandwidth.length; i += to_retain) {
    let item = node_stats.download_bandwidth[i];
    
    download_bandwidth_labels.push(format_time(item[0]));
    
    download_bandwidth_data.push(format_bytes_mb(item[1]));    
  }
   
  let upload_bandwidth_labels = [];
  let upload_bandwidth_data = [];
  for (let i = 0; i < node_stats.upload_bandwidth.length; i += to_retain) {
    let item = node_stats.upload_bandwidth[i];
    
    upload_bandwidth_labels.push(format_time(item[0]));
    
    upload_bandwidth_data.push(format_bytes_mb(item[1]));    
  }
  
  let options = {
    plugins: {
      legend: {
        display: false
      },
    },
    elements: {
      point:{
        radius: 0
      }
    },
    animation: false,
    scales: {
      x: {
        ticks: {
            display: true,
            autoSkip: true,
            maxTicksLimit: 5
        },
      },
    },
  };

  if (first_time) {
    total_nodes_chart = new Chart(document.getElementById('total-nodes'), {
      type: 'line',
      data: {
        labels: total_nodes_labels,
        datasets: [{
          label: 'nodes',
          data: total_nodes_data,
          tension: 0.2,
          fill: true,
          borderColor: '#FB7800',  
        }]
      },
  
      options: options,
    });
  
    online_nodes_chart = new Chart(document.getElementById('online-nodes'), {
      type: 'line',
      data: {
        labels: online_nodes_labels,
        datasets: [{
          label: 'nodes',
          data: online_nodes_data,
          tension: 0.2,
          fill: true,
          borderColor: '#FB7800',  
        }]
      },
  
      options: options,
    });


    storage_used_chart = new Chart(document.getElementById('storage-used'), {
      type: 'line',
      data: {
        labels: storage_used_labels,
        datasets: [{
          label: 'GB',
          data: storage_used_data,
          tension: 0.2,
          fill: true,
          borderColor: '#FB7800',  
        }]
      },
  
      options: options,
    });


    download_bandwidth_chart = new Chart(document.getElementById('download-bandwidth'), {
      type: 'line',
      data: {
        labels: download_bandwidth_labels,
        datasets: [{
          label: 'kb/s',
          data: download_bandwidth_data,
          fill: true,
          borderColor: '#FB7800',  
        }]
      },
  
      options: options,
    });

    upload_bandwidth_chart = new Chart(document.getElementById('upload-bandwidth'), {
      type: 'line',
      data: {
        labels: upload_bandwidth_labels,
        datasets: [{
          label: 'kb/s',
          data: upload_bandwidth_data,
          tension: 0.2,
          fill: true,
          borderColor: '#FB7800',  
        }]
      },
  
      options: options,
    });

    
  } else {
    total_nodes_chart.data.labels = total_nodes_labels;
    total_nodes_chart.data.datasets[0].data = total_nodes_data;

    online_nodes_chart.data.labels = online_nodes_labels;
    online_nodes_chart.data.datasets[0].data = online_nodes_data;
    
    storage_used_chart.data.labels = storage_used_labels;
    storage_used_chart.data.datasets[0].data = storage_used_data;
    
    download_bandwidth_chart.data.labels = download_bandwidth_labels;
    download_bandwidth_chart.data.datasets[0].data = download_bandwidth_data;
    
    upload_bandwidth_chart.data.labels = upload_bandwidth_labels;
    upload_bandwidth_chart.data.datasets[0].data = upload_bandwidth_data;
    
    total_nodes_chart.update();
    online_nodes_chart.update();
    storage_used_chart.update();
    download_bandwidth_chart.update();
    upload_bandwidth_chart.update();
  }

  document.getElementById("current-total-nodes").innerHTML = total_nodes_data[total_nodes_data.length - 1] + " nodes";
  document.getElementById("current-online-nodes").innerHTML = online_nodes_data[online_nodes_data.length - 1] + " nodes";
  document.getElementById("current-storage-used").innerHTML = format_bytes(node_stats.storage_used[node_stats.storage_used.length - 1][1]);
  document.getElementById("current-download-bandwidth").innerHTML = format_bytes(node_stats.download_bandwidth[node_stats.download_bandwidth.length - 1][1]) + "/s";
  document.getElementById("current-upload-bandwidth").innerHTML = format_bytes(node_stats.upload_bandwidth[node_stats.upload_bandwidth.length - 1][1]) + "/s";
}

async function update() {
  await fetch(api_url + "/get_pinned_resources")
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    resources = JSON.parse(data);

    set_resources();
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });

  await fetch(api_url + "/get_node_info")
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    node_info = JSON.parse(data);

    set_info();
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });

  await fetch(api_url + "/get_node_stats")
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    node_stats = JSON.parse(data);

    set_stats();
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });
  
}

window.onload = async function() {
  await main_load();
  
  await update();

  first_time = false;

  setInterval(update, 2000);

  setInterval(ping_backend, ping_rate);
}
