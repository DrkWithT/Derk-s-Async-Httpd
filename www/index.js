((doc) => {
    const replyDisplay = doc.querySelector('p#echo-txt')
    const userText = doc.querySelector('#usertxt');
    const submitButton = doc.querySelector('#send-btn');

    submitButton.addEventListener('click', async (event) => {
        if (!userText.value) {
            event.stopPropagation();
            event.preventDefault();
            return;
        }

        let response = await fetch('/', {
            method: 'POST',
            headers: {'Content-Type': 'text/plain'},
            body: `${userText.value}`
        }).catch(err => console.error($`AJAX Error: ${err}`)) || null;

        if (!response) {
            console.error('AJAX Error: failed to get any response.');
        } else if (response.ok) {
            replyDisplay.innerText = await response.text();
        } else {
            console.error(`AJAX Error: found a bad response code of ${response.status}`);
        }
    });
})(document);
