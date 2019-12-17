import requests

def download_file_from_google_drive(id):
    def get_confirm_token(response):
        for key, value in response.cookies.items():
            if key.startswith('download_warning'):
                return value

        return None

    URL = "https://docs.google.com/uc?export=download"

    session = requests.Session()

    response = session.get(URL, params = { 'id' : id }, stream = True)
    token = get_confirm_token(response)

    if token:
        params = { 'id' : id, 'confirm' : token }
        response = session.get(URL, params = params, stream = True)

    sys.stdout.buffer.write(response.content)


if __name__ == "__main__":
    import sys
    if len(sys.argv) is not 2:
        print("Usage: python gdrive.py drive_file_id")
    else:
        # TAKE ID FROM SHAREABLE LINK
        file_id = sys.argv[1]
        download_file_from_google_drive(file_id)
