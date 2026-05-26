# Pebble Best Practices: Packaging, Publishing & Updates

## PBW Packaging

The compiled app and resources are bundled into a `.pbw` file. Minimize it by:
- Compressing images (PNG with palette)
- Trimming unused sprite frames
- Removing debug symbols
- Limiting custom fonts (each consumes flash/RAM)

The App Store imposes a 50 MB upload limit, but individual apps must fit device constraints: 128 KB on Aplite, 256 KB on Basalt/Chalk【48†L64-L72】.

## Versioning

Bump version numbers in `package.json`. Users get updates via the Pebble (Rebble) app store or sideload. There is no "hot code push" — updates require a new PBW. For dynamic content, rely on timeline pins or phone-connected JS.

## Timeline Publishing

If your app uses Pebble's Timeline, enable it in the Developer Portal (which gives you API keys)【22†L84-L93】. Pins pushed in sandbox (development) reach testers; in production, use production keys【22†L98-L107】. Always handle user subscription preferences (using the JS subscriptions API) so pins aren't spammy.

## Checklist

- Double-check resource limits: keep under 128 KB (Aplite) / 256 KB (Basalt/Chalk).
- Compress all images (palette PNG) and trim unused assets.
- Remove debug symbols and verbose logging before release.
- Test sideload and store installations on all target devices.
- Update appstore metadata (version, description, screenshots).
- If using Timeline, configure sandbox vs production API keys.
- Handle user subscription preferences for timeline pins.
- Bump `package.json` version for every release.
