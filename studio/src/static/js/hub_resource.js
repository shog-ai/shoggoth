let resource;

let template_str = `
  <div class="resource-main">
    <div class="resource-user">
      <img class="user-img" src="{{hub_url}}/static/img/users/{{this.namespace}}.png"/>
      <b class="resource-title"><a class="resource-title-namespace" href="/hub?search={{namespace}}">{{namespace}}</a> / {{name}}</b>
    </div>

    <div class="attributes">
      <a href="/hub?{{this.category}}" class="tag">
        <img class="tag-icon" src="/static/img/{{this.category}}.svg"/>
        <div class="tag-txt">{{this.category}}</div>
      </a>

      {{#each this.papers}}
        <a href="/hub/resource/{{this}}" class="tag">Paper: {{this}}</a>
      {{/each}}

      {{#each this.libraries}}
        <a href="/hub/resource/{{this}}" class="tag">Library: {{this}}</a>
      {{/each}}

      {{#each this.datasets}}
        <a href="/hub/resource/{{this}}" class="tag">Dataset: {{this}}</a>
      {{/each}}

      {{#each this.tags}}
        <a href="/hub?tag={{this}}" class="tag">{{this}}</a>
      {{/each}}
    </div>

    <div class="date">Published on {{time_published}}</div>

    <div class="download-btn-parent">
      {{#if is_pinned}}
        <a href="/node" class="download-btn">Show in node</a>
      {{else}}
        <div onclick="download_resource_pressed('{{shoggoth_id}}')" class="download-btn">Download</div>
      {{/if}}
    </div>

     <div class="resource-content">
      <div class="resource-content-left">
        <div class="resource-content-info">
          <div class="label">
            <b class="label-left">Label:</b>
            <div class="label-right">{{label}}</div>
          </div>
  
          <div class="shoggoth-id">
            <div class="shoggoth-id-top"><img class="icon" src="/static/img/id.svg"/>Shoggoth ID:</div>
            <div class="shoggoth-id-down">{{shoggoth_id}}</div>
          </div>
          
          <div class="shoggoth-id">
            <div class="shoggoth-id-top"><img class="icon" src="/static/img/id.svg"/>Torrent:</div>
            <div class="shoggoth-id-down">{{torrent}}</div>
          </div>
  
          <div class="shoggoth-id">
            <div class="shoggoth-id-top"><img class="icon" src="/static/img/size.svg"/>Size:</div>
            <div class="shoggoth-id-down">{{size}}</div>
          </div>
  

        </div>

     
      </div>

      <div class="resource-content-right">
        <div class="resource-content-title">
          <div>Downloads last month</div>
          <div class="downloads-total">~</div>
        </div>

        <div class="resource-content-chart">
          <div class="chart-container">
            <canvas id="downloads"></canvas>
          </div>
        </div>
      </div>
    </div>

    <div class="readme">
      <div class="readme-title">README</div>

      <div class="readme-content">
      </div>
    </div>
  </div>
`;

async function set_resource() {
  resource.category = resource.category.toLowerCase();
  
  resource.hub_url = hub_url;

  const date = new Date(resource.time_published);
  const year = date.getFullYear();
  const month = date.getMonth();
  const day = date.getDate();
  const month_names = ["January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"];

  resource.time_published = `${day} ${month_names[month]} ${year}`;
  resource.size = format_bytes(resource.size);

  // console.log(resource);

  await fetch(api_url + "/is_resource_pinned/" + resource.shoggoth_id)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    if (data == "true") {
      resource.is_pinned = true;
    } else {
      resource.is_pinned = false;
    }
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });
  
  await fetch(hub_url + "/api/get_downloads_last_month/" + resource.shoggoth_id)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    resource.downloads = JSON.parse(data);
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });

  const template = Handlebars.compile(template_str);
  let compiled = template(resource);
    
  const parent = document.querySelector(".main-content");
  parent.innerHTML = compiled;


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
            display: false
        },
        grid: {
          display:false
        }
      },
      y: {        
        grid: {
          display:false
        }
      }
    },
  };
  
  let downloads_chart = new Chart(document.getElementById('downloads'), {
    type: 'line',
    data: {
      labels: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30],
      datasets: [{
        label: 'downloads',
        data: resource.downloads,
        tension: 0.4,
        fill: true,
        backgroundColor: '#FB78003D',  
        borderColor: '#FB7800',  
      }]
    },

    options: options,
  });

  let total_downloads = 0;
  for (const value of resource.downloads) {
    total_downloads += value;
  }

  document.querySelector(".downloads-total").innerHTML = total_downloads.toLocaleString();
  
  await fetch(hub_url + "/api/get_resource_readme/" + resource.namespace + "/" + resource.name)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    if (data == "no readme") {
      let readme_container = document.querySelector(".readme-content");
      readme_container.innerHTML = "no readme";
    } else {
      let converter = new showdown.Converter();
      converter.setFlavor('github');

      let readme_html = converter.makeHtml(data);

      let readme_container = document.querySelector(".readme-content");
      readme_container.innerHTML = readme_html;
    }
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });

  hljs.highlightAll();
}


async function load() {
  var path = window.location.pathname.split('/').slice(-2);
  let namespace = path[0];
  let name = path[1];
  
  await fetch(hub_url + "/api/get_resource_by_name/" + namespace + "/" + name)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    resource = JSON.parse(data);

    set_resource().then(() => {replace_links_with_divs(".readme", resource.alternative_base)});
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });  
}


window.onload = async function() {
  await load();

  setInterval(ping_backend, ping_rate);
}

async function download_resource_pressed(shoggoth_id) {
  // console.log("download resource pressed: " + shoggoth_);

  await fetch(api_url + "/node_add_resource/" + shoggoth_id)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    toast("Resource Pinned", "success");

    document.querySelector(".download-btn-parent").innerHTML = `<a href="/node" class="download-btn">Show in node</a>`;
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });

}
