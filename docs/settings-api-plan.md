# Common settings API plan

## Why it is needed

Current desktop contracts expose settings as `QWidget *` or `QDialog *`.
Android cannot render those objects, and a plugin runtime must not depend on Qt
Widgets merely to describe configuration.

The common settings API is therefore not a second settings store. It is a
UI-neutral description/controller placed between plugin or storage settings and
the platform-specific view.

## Recommended design

Use a hybrid contract:

1. A schema-driven model covers normal fields: boolean, string, integer, enum,
   path, password/secret, validation state, help text, and restart requirement.
2. The controller reads, validates, applies, and resets values. Secrets are
   represented by keys or opaque handles rather than exposed as plain model data.
3. Desktop renders the schema with Widgets or QML during migration; Android
   renders the same schema with QML.
4. A plugin may optionally provide a custom QML component for genuinely complex
   settings, but the component still talks to the common controller.

This choice should be agreed before changing the current
`settingsWidget()/optionsDialog()` plugin contracts.
