# rridley36.github.io

Personal GitHub Pages site for Russell Ridley — showcases the **Application 6** project for EEL 5862 Real-Time Systems (Spring 2026, UCF).

**Live site:** https://rridley36.github.io

## Contents

- `index.html` — single-page project showcase with overview, embedded YouTube demo, feature list, and links
- `style.css` — dark-themed, responsive styling

## Before publishing, fill in the placeholders

Search the files for `TODO` and replace:

1. **YouTube video ID** in `index.html` — the `src` of the `<iframe>` currently points to `https://www.youtube.com/embed/YOUTUBE_VIDEO_ID`. Replace `YOUTUBE_VIDEO_ID` with the ID from your YouTube URL (e.g. for `https://youtu.be/abc123XYZ`, the ID is `abc123XYZ`).
2. **Project description** paragraph inside the Overview section.
3. **Source repo link** and **YouTube link** in the Links section.

## Publishing with GitHub Pages

1. Create a new repository on GitHub named exactly `rridley36.github.io` (public).
2. Upload `index.html`, `style.css`, and `README.md` to the repo root — either via the GitHub web UI ("Add file → Upload files") or by pushing with git:
   ```bash
   git init
   git add .
   git commit -m "Initial site"
   git branch -M main
   git remote add origin https://github.com/rridley36/rridley36.github.io.git
   git push -u origin main
   ```
3. In the repo, go to **Settings → Pages**.
4. Under **Build and deployment**, set **Source** to **Deploy from a branch**, then choose **Branch: `main`** and folder **`/ (root)`**. Click **Save**.
5. Wait ~1 minute, then visit https://rridley36.github.io.

## Local preview

Just open `index.html` in a browser — no build step required.

## License

Personal academic project. All rights reserved.
