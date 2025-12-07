(function (doc) {
    const userText = doc.querySelector('textarea');
    const submitButton = doc.querySelector('button');

    submitButton.addEventListener('click', async () => {
        let response = await fetch('/', {
            method: 'POST',
            headers: {
                'Content-Type': 'text/plain'
            },
            body: `${userText.innerText}`
        }).catch(err => console.error($`AJAX Error: ${err}`)) || null;

        if (!response) {
            console.error('AJAX Error: failed to get any response.');
            return;
        }

        if (response.ok) {
            userText.innerText = await response.text();
        } else {
            console.error(`AJAX Error: found a bad response code of ${response.status}`);
        }
    });
})(document);
