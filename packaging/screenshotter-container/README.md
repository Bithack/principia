# Screenshotter Container
A container for doing automatic screenshotting with Principia. It builds a screenshotter build of Principia inside of an Alpine Linux container that is used primarily for taking screenshots of community levels.

## Setting up
Install Docker on both the development and server environment, development environment being where you'll build the container image and the server environment where it will be deployed.

On your development environment, build the container image and save it to a tarball.

```bash
docker build . -t principia-screenshotter:latest
docker image save principia-screenshotter:latest -o cuddles.tar
zstd -T0 cuddles.tar
```

Transfer `cuddles.tar.zst` to your server environment with rsync or similar.

In your server environment, load the tarball image:

```bash
sudo docker load cuddles.tar
```

Create a container from the image and run principia inside of it (container name is `ss` to be short):

```bash
screen -h 10000 -S screenshotter sudo docker run -i -t --name ss principia-screenshotter:latest /principia/principia
```

You can now detach from the screen. For an example of how to take screenshots see `tools/take_screenshot.sh` in principia-web.
