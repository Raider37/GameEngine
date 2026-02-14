using System.Globalization;

namespace ScriptRuntime;

internal static class Program
{
    private static readonly CultureInfo Ci = CultureInfo.InvariantCulture;

    private static int Main(string[] args)
    {
        if (args.Length != 2)
        {
            Console.Error.WriteLine("Usage: ScriptRuntime <inputFile> <outputFile>");
            return 1;
        }

        var inputPath = args[0];
        var outputPath = args[1];

        if (!File.Exists(inputPath))
        {
            Console.Error.WriteLine($"Input file not found: {inputPath}");
            return 2;
        }

        var lines = File.ReadAllLines(inputPath);
        if (lines.Length == 0)
        {
            File.WriteAllText(outputPath, string.Empty);
            return 0;
        }

        if (!float.TryParse(lines[0], NumberStyles.Float, Ci, out var deltaTime))
        {
            Console.Error.WriteLine($"Invalid delta time in input header: {lines[0]}");
            return 3;
        }

        var outputLines = new List<string>();

        for (var i = 1; i < lines.Length; i++)
        {
            var line = lines[i];
            if (string.IsNullOrWhiteSpace(line))
            {
                continue;
            }

            var parts = line.Split('|');
            if (parts.Length != 5)
            {
                continue;
            }

            if (!uint.TryParse(parts[0], NumberStyles.Integer, Ci, out var entityId))
            {
                continue;
            }

            if (!TryParseVec3(parts[2], parts[3], parts[4], out var position))
            {
                continue;
            }

            var scriptClass = parts[1];
            var behavior = ScriptFactory.Create(scriptClass);
            behavior?.OnUpdate(ref position, deltaTime);

            outputLines.Add(string.Create(Ci, $"{entityId}|{position.X:F6}|{position.Y:F6}|{position.Z:F6}"));
        }

        File.WriteAllLines(outputPath, outputLines);
        return 0;
    }

    private static bool TryParseVec3(string x, string y, string z, out Vec3 position)
    {
        if (!float.TryParse(x, NumberStyles.Float, Ci, out var px) ||
            !float.TryParse(y, NumberStyles.Float, Ci, out var py) ||
            !float.TryParse(z, NumberStyles.Float, Ci, out var pz))
        {
            position = default;
            return false;
        }

        position = new Vec3(px, py, pz);
        return true;
    }
}
