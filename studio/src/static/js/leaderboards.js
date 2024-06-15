let leaderboards;
let votes;

let template_str = `
{{#each this.leaderboards}}
  <div class="board-category-title">
    {{this.name}}
  </div>
  <div class="board-category">
    {{#each this.options}}
      <div id="{{this.id}}" class="board-category-item item-{{this.rank}}">
        <div class="item-number">{{this.rank}}</div>
        <div class="item-name"><img class="item-image" src="{{../../hub_url}}/static/img/users/{{this.namespace}}.png"/>{{this.namespace}}/{{this.name}}</div>
        <div class="item-votes">
          <div>{{this.votes}} votes</div>
          <div class="vote-btn" onclick="vote_pressed('{{../name}}', '{{this.namespace}}', '{{this.name}}')">vote</div>
        </div>
      </div>
    {{/each}}
  </div>
{{/each}}
`;

async function vote_pressed(category, namespace, name) {
  console.log("voting " + namespace + "/" + name + " for " + category);
  
  await fetch(api_url + "/leaderboard_register_vote/" + category + "/" + namespace + "/" + name)
  .then(async function (response) {
    if (!response.ok) {
      let body = await response.text();
      throw {"err": `HTTP error! Status: ${response.status}`, "body": body};
    }
    
    return response.text();
  })
  .then(data => {
    toast("Vote Successful", "success");
    
    sleep(3000).then(() => { reload() });
  })
  .catch(error => {
     toast_big("Vote Failed", error.body);
     
    console.error('Fetch error:', error);
  });  
}

function set_boards() {
  // console.log(leaderboards);

  leaderboards.forEach((category_item) => {
    
    category_item.options.forEach((item) => {
      let element_id = category_item.name.split(' ').map(word => word.toLowerCase()).join('-') + "_" + item.namespace + "_" + item.name;
      // console.log(element_id);
      item.id = element_id;
    });
  
  });

  let template_data = {leaderboards, hub_url};
  
  const template = Handlebars.compile(template_str);
  let compiled = template(template_data);

  const parent = document.querySelector(".main-content");
  parent.innerHTML = compiled;
 
  votes.forEach((item) => {
    // console.log(item);

    let element_id = item[0].split(' ').map(word => word.toLowerCase()).join('-') + "_" + item[1].namespace + "_" + item[1].name;
    // console.log(element_id);

    let vote_btn = document.getElementById(element_id);
    vote_btn.querySelector(".vote-btn").innerHTML = "voted";
    vote_btn.querySelector(".vote-btn").classList.add("vote-btn-disabled");
  });
}

async function load() {  
  await fetch(hub_url + "/api/get_leaderboards")
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    leaderboards = JSON.parse(data);
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });  


  for (let i = 0; i < leaderboards.length; i++) {
    leaderboards[i].options.sort((a, b) => b.votes - a.votes);
    
    for (let k = 0; k < leaderboards[i].options.length; k++) {
      leaderboards[i].options[k].rank = k + 1;
      leaderboards[i].options[k].votes = leaderboards[i].options[k].votes.toLocaleString();
    }
  } 
     
  await fetch(hub_url + "/api/get_node_leaderboard_votes/" + node_id)
  .then(response => {
    if (!response.ok) {
      throw new Error(`HTTP error! Status: ${response.status}`);
    }
    
    return response.text();
  })
  .then(data => {
    // console.log(data);

    votes = JSON.parse(data);
  })
  .catch(error => {
    console.error('Fetch error:', error);
  });  
  
  // console.log(leaderboards);

  set_boards();
}


window.onload = async function() {
  await main_load();
  
  await load();

  setInterval(ping_backend, ping_rate);
}

