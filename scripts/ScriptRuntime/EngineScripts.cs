using System.Globalization;

namespace ScriptRuntime;

public struct Vec3
{
    public float X;
    public float Y;
    public float Z;

    public Vec3(float x, float y, float z)
    {
        X = x;
        Y = y;
        Z = z;
    }

    public override string ToString()
    {
        return string.Create(CultureInfo.InvariantCulture, $"{X:F3},{Y:F3},{Z:F3}");
    }
}

public interface IScriptBehavior
{
    void OnUpdate(ref Vec3 position, float deltaTime);
}

public sealed class MoveForwardScript : IScriptBehavior
{
    public void OnUpdate(ref Vec3 position, float deltaTime)
    {
        position.Z += 4.0f * deltaTime;
    }
}

public sealed class OrbitYScript : IScriptBehavior
{
    public void OnUpdate(ref Vec3 position, float deltaTime)
    {
        var radius = MathF.Max(0.001f, MathF.Sqrt((position.X * position.X) + (position.Z * position.Z)));
        var angle = MathF.Atan2(position.Z, position.X) + (1.2f * deltaTime);
        position.X = MathF.Cos(angle) * radius;
        position.Z = MathF.Sin(angle) * radius;
    }
}

public static class ScriptFactory
{
    public static IScriptBehavior? Create(string scriptClass)
    {
        return scriptClass switch
        {
            nameof(MoveForwardScript) => new MoveForwardScript(),
            nameof(OrbitYScript) => new OrbitYScript(),
            _ => null
        };
    }
}
