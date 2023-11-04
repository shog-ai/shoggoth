console.log("Hello anon!");

function explorer_search() {
  const input_field = document.getElementById("search-input");

  search_id = input_field.value;

  window.location.href = '/explorer/profile/' + search_id;
}

function handle_search_key_press(event) {
  if (event.key === "Enter") {
    explorer_search();
  }
}

