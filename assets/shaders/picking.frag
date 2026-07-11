#version 420 core
layout (location = 0) out uvec3 outID;

uniform uint ObjectID;
uniform uint DrawID;
uniform uint PrimID;

void main()
{
    outID = uvec3(ObjectID, DrawID, PrimID);
}
