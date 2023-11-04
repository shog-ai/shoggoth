
function open_sidenav() {
  const sidenav = document.getElementById('sidenav');
  
  sidenav.classList.add('sidenav-expanded');
}

function close_sidenav() {
  const sidenav = document.getElementById('sidenav');
  
  sidenav.classList.remove('sidenav-expanded');
}